#include <cstring>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#include "sqlite.hpp"

using std::stringstream;
using std::string;

namespace sqlite
{

Query::Query(sqlite3* handle) : db(handle), error_code(0), stmt(NULL)
{
    
}

Query::~Query()
{
    finalize();
}

int Query::execute(const char* sql, ...)
{
    char *err, *tmp;

    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);

    error_code = sqlite3_exec(db, tmp, NULL, NULL, &err);

    if(error_code != SQLITE_OK)
    {
        if (err != NULL)
        {
            fprintf(stderr, "execute() Error: %s\n", err);
            sqlite3_free(err);
        }
    }

    sqlite3_free(tmp);

    return error_code;
}

bool Query::exists(const char* sql, ...)
{
    const char *tail;
    char *tmp;

    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);

    if(stmt != NULL)
    {
        finalize();
    }

    error_code = sqlite3_prepare_v2(db, tmp, strlen(tmp), &stmt, &tail);
    sqlite3_free(tmp);

    bool result = false;

    if(error_code != SQLITE_OK)
    {
        result = false;
    }

    result = (step() == SQLITE_ROW);

    finalize();

    return result;
}

void Query::prepare(const char* sql, ...)
{
    const char *tail;
    char *tmp;

    va_list ap;
    va_start(ap, sql);
    tmp = sqlite3_vmprintf(sql, ap);
    va_end(ap);

    if(stmt != NULL)
    {
        finalize();
    }

    error_code = sqlite3_prepare_v2(db, tmp, strlen(tmp), &stmt, &tail);
    sqlite3_free(tmp);

    if(error_code != SQLITE_OK)
    {
        std::stringstream stream;
        stream << " [" << error_code << "] " 
               << "Query::prepare() failed --"
               << sqlite3_errmsg(db) << " SQL: " << sql;
        
        std::runtime_error e(stream.str());
        throw e;
    }
}

int32_t Query::step()
{
    return (error_code = sqlite3_step(stmt));
}

void Query::finalize()
{
    if(stmt != NULL)
    {
        sqlite3_finalize(stmt);
        stmt = NULL;
    }
}

std::string Query::error(const char* msg, ...)
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

    text += sqlite3_errmsg(db);
    
    return text;
}

} // ! namespace sqlite
