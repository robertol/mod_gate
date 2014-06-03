#ifndef MODJERK_SQLITE_DECL
#define MODJERK_SQLITE_DECL

#include <stdint.h>
#include "sqlite3.h"

namespace sqlite
{

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
