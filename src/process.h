#ifndef MODGATE_APACHE_PROCESS_DECL
#define MODGATE_APACHE_PROCESS_DECL

#include <httpd.h>
#include <http_request.h>
#include <http_config.h>
#include <http_core.h>
#include <http_protocol.h>

namespace apache
{

class Process
{
    Process();

  protected:
    
    process_rec* _r;

  public:

    Process(process_rec* r);
    ~Process();

    inline apr_pool_t* pool() { return _r->pool; }
    inline apr_pool_t* pconf() { return _r->pconf; }
    inline int argc() { return _r->argc; }
    inline const char* const* argv() { return _r->argv; }
    inline const char* short_name() { return _r->short_name; }

};

} // end namespace apache

#endif
