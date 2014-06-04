#ifndef MODJERK_APACHE_MODULE_HPP_DECL
#define MODJERK_APACHE_MODULE_HPP_DECL

#include "request.h"
#include "server.h"
#include "module.h"
#include "config.h"

// This is for C++ functions that can't be included in module.h (which is
// strictly C).

namespace sqlite
{

class Database
{
    // Disallow assignment operator and default copy ctor
    const Database &operator=(const Database &old); 
    Database(const Database &old);

    sqlite3* db; ///< Filter database

  public:

    Database(sqlite3* x);
    virtual ~Database();

    inline sqlite3* handle() const { return db; } 

    /// @name Database operations
    /// @{
    
    /// Executes a general SQL statement.
    /// @param sql The SQL statement
    /// @param ... Variadic arguments. Arguments are formatted using sqlite3_vmprintf().
    /// @return Returns the status code from sqlite3_exec()
    int32_t execute(const char* sql, ...);

    /// Returns a count for an SQL statement. Selects first col of first row
    /// using sqlite_column_int().
    /// @param sql The SQL statement
    /// @param ... Variadic arguments. Arguments are formatted using sqlite3_vmprintf().
    /// @return Returns the status code from sqlite3_exec()
    int32_t count(const char* sql, ...);

    /// Returns first value for first record an SQL statement. Selects first col
    /// of first row using sqlite_column_text().
    /// @param sql The SQL statement
    /// @param ... Variadic arguments. Arguments are formatted using sqlite3_vmprintf().
    /// @return Returns the status code from sqlite3_exec()
    std::string value(const char* sql, ...);

    /// This is a variadic method which returns a custom formatted message for
    /// last DB error. Appends sqlite3_errormg() to it.
    ///
    /// @param db database handle
    /// @param msg printf-style string 
    /// @param ... Variadic arguments supplied to params specified in
    ///        msg. Arguments are formatted using sqlite3_vmprintf().
    static std::string error(sqlite3 *db, const char* msg, ...);

    //@}    
};

} // end namespace sqlite


namespace modjerk
{

class Handler
{
    std::string my_database_path;
    std::string my_error_message;
    sqlite::Database* my_database;
    
  public:

    Handler(const char* database = ""); 
    ~Handler();

    // Unique key/name/id of this handler
    const char* id()
    {
        return my_database_path.c_str();
    }

    const char* databaseFile() const
    {
        return my_database_path.size() == 0 ? NULL : my_database_path.c_str();
    }

    const std::string& error() const
    {
        return my_error_message;
    }

    const sqlite::Database* database() const
    {
        return my_database;
    }

    bool connect();
};

} // end namespace modjerk

modjerk::Handler jerk_request_get_handler(request_rec* r);


#endif
