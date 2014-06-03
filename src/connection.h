#ifndef MODJERK_CONNECTION_DECL
#define MODJERK_CONNECTION_DECL

#ifdef WIN32
#pragma warning( disable : 4275 4273 4786 )
#ifdef EXPORT_MODCPP
#define MODCPP_API __declspec(dllexport)
#else
#define MODCPP_API __declspec(dllimport)
#endif /* EXPORT_MODCPP */
#else
#define MODCPP_API
#endif /* WIN32 */

#include <httpd.h>
#include <http_request.h>
#include <http_config.h>
#include <http_core.h>
#include <http_protocol.h>
#include <apr_strings.h>

#include <string>

using std::string;

namespace apache
{

class MODCPP_API Connection
{
  protected:
    
    conn_rec* _c;

    Connection();

  public:

    Connection(conn_rec* c);
    ~Connection();

    inline apr_pool_t* pool() { return _c->pool; }

    inline unsigned aborted() const
    {
        return _c->aborted;
    }

    inline int double_reverse() const
    {
        return _c->double_reverse;
    }

    inline int id() const
    {
        return _c->id;
    }

    ap_conn_keepalive_e keepalive() const
    {
        return _c->keepalive;
    }

    inline const char* local_ip() const
    {
        return _c->local_ip;
    }

    inline int local_port() const
    {
        return _c->local_addr->port;
    }

    inline const char* local_host() const
    {
        return _c->local_host;
    }

    inline apr_table_t* notes() const
    {
        return _c->notes;
    }

    inline const char* remote_host() const
    {
        return _c->remote_host;
    }

    inline const char* remote_ip() const
    {
#if AP_SERVER_MINORVERSION_NUMBER < 4
        return _c->remote_ip;
#else
        return _c->client_ip;
#endif
    }

    inline int remote_port() const
    {
#if AP_SERVER_MINORVERSION_NUMBER < 4
        return _c->remote_addr->port;
#else
        return _c->client_addr->port;
#endif
    }

    inline const char* remote_logname() const
    {
        return _c->remote_logname;
    }

    inline const server_rec* server() const
    {
        return _c->base_server;
    }

};

} // end namespace apache

#endif
