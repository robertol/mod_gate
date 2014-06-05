#include <arpa/inet.h>
#include <unistd.h>

#include <stdlib.h>

#include <sstream>
#include <fstream>

#include "platform.h"
#include "getopt.h"
#include "sqlite.hpp"

using namespace sqlite;
using std::string;
using std::endl;
using std::stringstream;

int global_verbosity = 0;

//------------------------------------------------------------------------------
// Command line options
//------------------------------------------------------------------------------

static struct option opt_array[] = {
    { "create",    0,  NULL, 'c', "Create database"       },
    { "index",     0,  NULL, 'i', "Index database"        },
    { "lint",      0,  NULL, 'l', "Lint database"         },
    { "verbosity", 1,  NULL, 'v', "Verbosity"             },
    { "help",      0,  NULL, 'h', "Show help"             },
    {  NULL,       0,     0,  0,   NULL                   }
};

void print_help()
{
    printf( "jerkdb version %s (%s)\n\n"
            "Usage: jerkdb [options]\n\n"
            "Available options:\n\n",
            MODJERK_RELEASE_VERSION, SYSTEM_ARCH );

    const option* i = opt_array;

    while(1)
    {
        if(i->name == NULL)
        {
            break;
        }

        if(i->has_arg != 0)
        {
            stringstream long_opt;
            long_opt << "--" << i->name << "=<" << i->name << ">";
            printf( "   -%c, %-20s %s\n", i->val, 
                    long_opt.str().c_str(), i->description);
        }
        else
        {
            printf("   -%c, --%-18s %s\n", i->val, i->name, i->description);
        }

        i++;
    }
    
    printf("\n");
}

sqlite3* open_database(const char* path)
{
    // Initialize the database
    sqlite3* sqlite_db_handle;
    int rc = sqlite3_open(path, &sqlite_db_handle);
    
    if(rc)
    {
        stringstream strm;
        strm << Database::error(sqlite_db_handle, "Can't open database: ")
             << path;
        
        fprintf(stderr, "%s\n", strm.str().c_str());

        // Close DB
        sqlite3_close(sqlite_db_handle);

        return NULL;
    }

    return sqlite_db_handle;
}

int create_database(const char* path)
{
    const char* db_sql = 
        "CREATE TABLE ua (          \n"
        "    id integer primary key,\n" 
        "    name text,             \n"
        "    http_code int,         \n"
        "    http_message text );   \n"    
        "CREATE TABLE ip (          \n"
        "    id integer primary key,\n"
        "    start_text text,       \n"
        "    end_text text,         \n"
        "    start int,             \n"
        "    end int,               \n"
        "    http_code int,         \n"
        "    http_message text );   \n" ;
    
    if(access(path, R_OK) == 0)
    {
        fprintf(stderr, "Error: database file already exists: %s\n", path);
        return 1;
    }

    sqlite3* sqlite_db_handle = open_database(path);

    if(sqlite_db_handle == NULL)
    {
        return 1;
    }

    Database db(sqlite_db_handle);

    if(db.execute(db_sql) != SQLITE_OK)
    {
        string db_error = Database::error(db.handle(), "Failed to create database");
        stringstream msg;
        msg << "Failed to create database: "
            << ": " << db_error;
        
        fprintf(stderr, "%s\n", msg.str().c_str());

        return 1;
    }

    return 0;
}

int index_database(const char* path)
{
    sqlite3* sqlite_db_handle = open_database(path);

    if(sqlite_db_handle == NULL)
    {
        return 1;
    }

    Database db(sqlite_db_handle);
    Query query(db.handle());
    Query updater(db.handle());

    // Select all queues
    std::stringstream sql;
    sql << "SELECT id, start_text, end_text FROM ip "
        << "WHERE length(start)=0 OR length(end)=0";
    
    query.prepare(sql.str().c_str());
    
    while(query.step() == SQLITE_ROW)
    {
        int32_t record_id      = sqlite3_column_int(query.stmt, 0);
        const char* start_text = (char*)sqlite3_column_text(query.stmt, 1);
        const char* end_text   = (char*)sqlite3_column_text(query.stmt, 2);
        
        fprintf(stderr, "%s %s\n", start_text, end_text);

        struct in_addr ipvalue;
        if(inet_pton(AF_INET, start_text, &ipvalue) != 1)
        {
            std::stringstream msg;
            msg << "Failed to convert IP address for record: " << record_id
                << ": " << start_text;           
        }
        
        // Convert to network byte order
        uint32_t start_addr = htonl(ipvalue.s_addr);

        if(inet_pton(AF_INET, end_text, &ipvalue) != 1)
        {
            std::stringstream msg;
            msg << "Failed to convert IP address for record: " << record_id
                << ": " << end_text;
            
            fprintf(stderr, "%s\n", msg.str().c_str());
        }
        
        // Convert to network byte order
        uint32_t end_addr = htonl(ipvalue.s_addr);

        sql.clear();

        sql.str(std::string());
        sql << "UPDATE ip set start=" << start_addr << ","
            << "end=" << end_addr << " "
            << "WHERE id=" << record_id;

        if(global_verbosity >= 1)
        {
            fprintf(stderr, "%s\n", sql.str().c_str());
        }

        if(db.execute(sql.str().c_str()) != SQLITE_OK)
        {
            std::stringstream msg;
            msg << "Failed to execute query for record: " << record_id
                << ": " << sql.str().c_str();
            
            fprintf(stderr, "%s\n", msg.str().c_str());
        }
    }
    
    return 0;
}

int lint_database(const char* path)
{
    sqlite3* sqlite_db_handle = open_database(path);

    if(sqlite_db_handle == NULL)
    {
        return 1;
    }

    Database db(sqlite_db_handle);
    Query query(db.handle());
    Query updater(db.handle());

    // Select all queues
    std::stringstream sql;
    sql << "SELECT id, start_text, end_text FROM ip "
        << "WHERE length(start)=0 OR length(end)=0";
    
    query.prepare(sql.str().c_str());
    
    while(query.step() == SQLITE_ROW)
    {
        int32_t record_id      = sqlite3_column_int(query.stmt, 0);
        const char* start_text = (char*)sqlite3_column_text(query.stmt, 1);
        const char* end_text   = (char*)sqlite3_column_text(query.stmt, 2);
        
        fprintf(stderr, "Unindexed IP: id=%i\n", record_id);
    }
    
    return 0;
}

int process_cmdline(int argc, char* const argv[])
{
    char ch;
    int long_opt_index = 0;
    bool arg_provided  = false;

    while((ch = getopt_long(argc, argv, "cilv:h", opt_array, &long_opt_index)) != -1)
    {
        switch(ch)
        {
            case 'c':
            {
                return create_database(argv[optind]);
            }
            
            case 'i':
            {
                return index_database(argv[optind]);
            }

            case 'l':
            {
                return lint_database(argv[optind]);
            }

            case 'v':
            {
                global_verbosity = atoi(optarg);
                continue;
            }

            case 'h':
            {
                print_help();

                return 0;
            }
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    return process_cmdline(argc, argv);
}
