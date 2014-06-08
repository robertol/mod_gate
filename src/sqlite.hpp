#ifndef MODGATE_SQLITE_DECL
#define MODGATE_SQLITE_DECL

#include <stdint.h>
#include "sqlite3.h"

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

/// A simple SQLite query wrapper.
class Query
{
    // Disallow assignment operator and default copy ctor
    const Query &operator=(const Query &old); 
    Query(const Query &old);

    sqlite3* db; ///< Database handle

public:

    int error_code; ///< The last error code
    sqlite3_stmt* stmt; ///< Statement handle

    /// Requires (open) database handle
    Query(sqlite3* handle);
    virtual ~Query();

    /// Corresponds to sqlite3_prepare()
    void prepare(const char* sql, ...);

    /// Corresponds to sqlite3_step()
    int32_t step();

    /// Corresponds to sqlite3_finalize()
    void finalize();
    
    /// Executes general statement
    int execute(const char* sql, ...);

    /// Returns true if there is at least one row in result set, false otherwise.
    bool exists(const char* sql, ...);

    /// Returns formatted error message
    std::string error(const char* msg, ...);
};

} // end namespace sqlite

#endif
