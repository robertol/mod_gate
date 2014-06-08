#ifndef MODGATE_APACHE_MODULE_HPP_DECL
#define MODGATE_APACHE_MODULE_HPP_DECL

#include "request.h"
#include "server.h"
#include "module.h"
#include "config.h"

// This is for C++ functions that can't be included in module.h (which is
// strictly C).

namespace modgate
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

} // end namespace modgate

modgate::Handler gate_request_get_handler(request_rec* r);


#endif
