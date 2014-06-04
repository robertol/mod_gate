#include <arpa/inet.h>
#include <unistd.h>
#include <httpd.h>
#include <http_log.h>

#include <map>
#include <string>
#include <sstream>
#include <stdexcept>

#include "request.h"
#include "connection.h"
#include "server.h"
#include "config.h"
#include "sqlite.hpp"

#include "module.hpp"

#if AP_SERVER_MINORVERSION_NUMBER >= 4
APLOG_USE_MODULE();
#endif

using namespace std;

using namespace modjerk;
using namespace sqlite;

modjerk::Handler::Handler(const char* database) 
    : my_database(NULL), my_error_message()
{
    if(database != NULL)
    {
        my_database_path = database;
    }    
}

modjerk::Handler::~Handler()
{
    if(my_database != NULL)
    {
        delete my_database;
        my_database == NULL;
    }   
}

bool modjerk::Handler::connect()
{    
    // Initialize the database
    sqlite3* sqlite_db_handle;
    int rc = sqlite3_open(my_database_path.c_str(), &sqlite_db_handle);
    
    if(rc)
    {
        stringstream strm;
        strm << Database::error(sqlite_db_handle, "Can't open database: ")
             << my_database_path;
        
        my_error_message = strm.str();

        // Close DB
        sqlite3_close(sqlite_db_handle);

        return false;
    }

    my_database = new Database(sqlite_db_handle);

    return true;
}

void log_message(apr_pool_t* pool, int level, const char* message)
{
    stringstream strm;

    strm << "mod_jerk[" << getpid() << "]: "
         << message ;

    ap_log_perror(APLOG_MARK, level, 0, pool, strm.str().c_str());
}

void log_error(apr_pool_t* pool, const char* message)
{
    log_message(pool, APLOG_CRIT, message);
}

void log_warning(apr_pool_t* pool, const char* message)
{
    log_message(pool, APLOG_WARNING, message);
}

int jerk_init_module(apr_pool_t* p, server_rec* server)
{
    ap_log_perror( APLOG_MARK, APLOG_NOTICE, 0, p, 
                   "mod_jerk[%i]: startup_module", getpid() );

    int x = getpid();

    return 0;
}

int jerk_shutdown_module()
{
    return 0;
}

int jerk_log_error(request_rec* r, int level, const char* msg)
{    
    // Create a C++ request object, for convenience
    apache::Request request(r);
    
    // Create the error message
    stringstream strm;
    strm << "ModJerk: " << msg;
    
    // Print error to content
    request.rputs(strm.str().c_str());
    
    // Log error 
    ap_log_error( APLOG_MARK, level, 0, r->server, 
                  "mod_jerk[%i] : %s", 
                  getpid(), strm.str().c_str() );

    return 0;
}

bool merge_var( request_rec* r, jerk_dir_config* dir_cfg, apr::table& t, 
                const char* var_name, const char* server_config)
{
    // Look for existence of value in dir_config
    if(apr_table_get(dir_cfg->options, var_name) != NULL)
    {
        t.set(var_name, apr_table_get(dir_cfg->options, var_name));
    }
    else
    {
        // If not there, set from server_config, if exists.
        if(server_config != NULL)
        {
            t.set(var_name, server_config);
        }
    }
    
    // Now check to see if it was set in either case. If not, return false
    // signifying that the value was NOT set.
    if(t.get(var_name) == NULL)
    {
        return false;
    }

    return true;
}

// Processes configuration file options.
//
// This happens AFTER directory/location merge.
//
// All variables are merged into the request notes field, which is the final
// version of all information needed to process the request.
//
// Directory config options override server config options.

int jerk_request_init_configuration(request_rec* r)
{
    apache::Request req(r);

    // Pass in module config params
    apr::table notes         = req.notes();
    jerk_config* cfg         = jerk_server_config(r->server->module_config);
    jerk_dir_config* dir_cfg = jerk_directory_config(r);
    
    // Merge in user-defined options from server and dir configurations
    apr::table server_options(cfg->options);
    apr::table dir_options(dir_cfg->options);

    // Copy in the settings from the server config.
    apr::table::iterator ti(server_options);
    while(ti.next())
    {
        notes.set(ti.key(), ti.data());
    }
    
    // Now copy in the settings from directory config.
    ti = dir_options.begin();
    
    while(ti.next())
    {
        notes.set(ti.key(), ti.data());
    }    
    
    // ---------------------------------------------------------------------
    // Check for modules configuration variables
    // ---------------------------------------------------------------------   

    // These check for the server_config value and then the dir_config. If the
    // dir_config exists, it will overwrite the server_config.

    // Module
    if(!merge_var(r, dir_cfg, notes, "JerkDefaultDatabase", cfg->default_database))
    {
        //jerk_log_error(r, APLOG_CRIT, "JerkDefaultDatabase not defined.");
        //return -1;
    }
    
    // JerkFilter is optional.
    if(merge_var(r, dir_cfg, notes, "JerkFilter", cfg->handler))
    {
        // It is defined. So merge all of the handler's settings into the notes
        // table.
        apr_hash_t* h_config = jerk_handler_config(r, notes.get("JerkFilter"));

        if(h_config != NULL)
        {
            apr_hash_index_t *hi;
            char *key;
            apr_ssize_t klen;
            char* data;
            
            for(hi = apr_hash_first(r->pool, h_config); hi; hi=apr_hash_next(hi))
            {
                apr_hash_this(hi, (const void **)&key, &klen, (void **)&data);                
                notes.set(key, data);

                // Check that path exists and file can be read
                if((string)key == "JerkFilterDatabase")
                {
                    if(access(data, R_OK) != 0)
                    {
                        jerk_log_error( r, 
                                        APLOG_CRIT, 
                                        "JerkDefaultDatabase unreadable or does not exists." );
                        return -1;                        
                    }
                }
            }
        }
    }

    return 0;
}

// Gets selected handler
modjerk::Handler jerk_request_get_handler(request_rec* r)
{
    /* Handler resolution. Look for JerkHandler directive in dir_config or
    ** server_config. Handler must be a registered CustomHandler. If not
    ** present, use JerkDefaultHandler.
    **
    ** Handler can be in server or dir config. dir overrides server.
    */

    apache::Request req(r);

    // This contains all the configuration options
    apr::table notes = req.notes();

    jerk_config* cfg = jerk_server_config(r->server->module_config);

    // Use default server config
    const char* database = cfg->default_database;

    // Check for JerkFilter
    if(notes.get("JerkFilter") != NULL)
    {
        apr_hash_t* h_config;
        h_config = jerk_handler_config(r, notes.get("JerkFilter"));

        // If it doesn't exist
        if(h_config != NULL)
        {
            database = (const char*)apr_hash_get( h_config, 
                                                  "JerkFilterDatabase", 
                                                  APR_HASH_KEY_STRING );

            return modjerk::Handler(database);
        }
    }
    else
    {
        log_error(req.pool(), "JERK: JerkFilter defined");

        if(notes.get("JerkFilterDatabase") != NULL)
        {
            database = notes.get("JerkDefaultDatabase");
        }

        if(database == NULL)
        {
            std::logic_error e("Default database is not defined.");
            throw e;
        }

        // Add this for reference -- it is associated with the request
        notes.set("JerkFilterDatabase", database);

        return modjerk::Handler(database);
    }
    
    // Empty handler -- signifies error
    return modjerk::Handler();
}

int jerk_request_handler(request_rec* r)
{
    apache::Request req(r);
    
    try
    {
        // Load configuration
        if(jerk_request_init_configuration(r) != 0)
        {
            // Configuration failed
            return DECLINED;
        }
   
        modjerk::Handler handler = jerk_request_get_handler(r);

        if(handler.databaseFile() == NULL)
        {
            return DECLINED;
        }

        if(handler.connect() == false)
        {
            // Create error message                
            //fprintf(stderr, "Database Exception: %s\n", status_text);
        
            stringstream strm;
            strm << "FATAL ERROR: " << handler.error();

            log_error(req.pool(), strm.str().c_str());
            
            return 1;
        }
        /*
        else
        {
            ap_log_error( APLOG_MARK, APLOG_NOTICE, 0, r->server, 
                          "mod_jerk[%i]: connected to %s", 
                          getpid(), handler.databaseFile());            
        }
        */
        
        apache::Connection con(req.connection());
        apr::table headers = req.headers_in();
        
        const char* user_agent = headers.get("User-Agent");

        if(user_agent == NULL)
        {
            user_agent = "NIL";
        }

        //> Process the request
        
        struct in_addr ipvalue;
        if(inet_pton(AF_INET, con.remote_ip(), &ipvalue) != 1)
        {
            ap_log_error( APLOG_MARK, APLOG_CRIT, 0, r->server, 
                          "mod_jerk[%i]: inet_pton failed() ip=%s", 
                          getpid(), con.remote_ip());
            
            return DECLINED;
        }
        
        // Convert to network byte order
        uint32_t addr = htonl(ipvalue.s_addr);

        /*
        // Log error (critical)
        ap_log_error( APLOG_MARK, APLOG_NOTICE, 0, r->server, 
                      "mod_jerk[%i]: remote ip:=%s (%u) agent: %s", 
                      getpid(), con.remote_ip(), addr, user_agent );
        */
        
        // Look for IP match
        
        std::stringstream sql;
        
        // Select all queues
        sql << "SELECT id, http_code, http_message FROM ip "
            << "WHERE start>=" << addr << " "
            << "AND end<=" << addr << " "
            << "LIMIT 1";
        
        Query query(handler.database()->handle());
        query.prepare(sql.str().c_str());
        
        while(query.step() == SQLITE_ROW)
        {
            int32_t record_id = sqlite3_column_int(query.stmt, 0);
            int32_t http_code = sqlite3_column_int(query.stmt, 1);
            req.set_status(http_code);
            
            ap_log_error( APLOG_MARK, APLOG_NOTICE, 0, r->server, 
                          "mod_jerk[%i]: MATCH ip=%s record=%i http=%i", 
                          getpid(), con.remote_ip(), record_id, http_code);
            
            const unsigned char* text = sqlite3_column_text(query.stmt, 2);
            
            if(http_code == 200)
            {
                // Allow request processing to proceed.
                return DECLINED;
            }
            
            // Stop it here
            return OK;
        }
        
        query.finalize();
    }
    catch(const std::exception &e)
    {
        // Create a C++ request object, for convenience
        apache::Request request(r);
        
        // Print error to content
        request.rprintf("mod_jerk Error: %s", e.what());
        
        // Log error (critical)
        ap_log_error( APLOG_MARK, APLOG_CRIT, 0, r->server, 
                      "mod_jerk[%i] : %s", 
                      getpid(), e.what() );
        
        return OK;
    }
        
    // We are not doing anything
    return DECLINED;
}

//------------------------------------------------------------------------------
// Database
//------------------------------------------------------------------------------

Database::Database(sqlite3* x) : db(x)
{
    
}

Database::~Database()
{
    sqlite3_close(db);    
}

i32 Database::count(const char* sql, ...)
{
    char *tmp;
    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);
    
    Query query(db);
    query.prepare(tmp);

    sqlite3_free(tmp);
    query.step();

    return sqlite3_column_int(query.stmt, 0);
}

std::string Database::value(const char* sql, ...)
{
    char *tmp;
    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);
    
    Query query(db);

    query.prepare(tmp);

    sqlite3_free(tmp);
    query.step();

    const char* field = (char*)sqlite3_column_text(query.stmt, 0);

    if(field == NULL)
    {
        return string();
    }

    return field;
}

int Database::execute(const char* sql, ...)
{
    char *err, *tmp;

    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);

    int rc = sqlite3_exec(db, tmp, NULL, NULL, &err);

    if(rc != SQLITE_OK)
    {
        if (err != NULL)
        {
            fprintf(stderr, "execute() Error: %s\n", err);
            sqlite3_free(err);
        }
    }

    sqlite3_free(tmp);

    return rc;
}

std::string Database::error(sqlite3* sdb, const char* msg, ...)
{
    std::string text;

    if(msg)
    {
        va_list ap;
        va_start(ap, msg);
        char* tmp = sqlite3_vmprintf(msg, ap);
        va_end(ap);

        text = tmp;
        sqlite3_free(tmp);
    }

    text += sqlite3_errmsg(sdb);
    
    return text;
}
