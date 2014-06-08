#ifndef MODGATE_MODULE_DECL
#define MODGATE_MODULE_DECL

#include <http_protocol.h>
#include <http_config.h>

#include "common.h"

#include <apr_hash.h>

BEGIN_DECL

/** 
This structure holds the configuration parameters in the Apache module.
*/
typedef struct gate_config
{
    char* handler;                  ///< Current handler selected for given request
    char* default_database; ///< Default handler database

    apr_table_t* options;          ///< Apache handler user-defined options to pass in
    apr_hash_t* handlers;          ///< User-defined Apache handlers 

} gate_config;

/** 
This structure holds directory configuration parameters in the GATE Apache module.
*/
typedef struct gate_dir_config
{
    char* dir;
    apr_table_t* options;
} gate_dir_config;

extern module AP_MODULE_DECLARE_DATA gate_module;

gate_config* gate_server_config(ap_conf_vector_t* module_config);
gate_dir_config* gate_directory_config(request_rec* r);
apr_hash_t* gate_handler_config(request_rec* r, const char* name);

/* Main module functions */
int gate_init_module(apr_pool_t* p, server_rec* s);
int gate_shutdown_module();
int gate_request_handler(request_rec* req);
int gate_request_init_configuration(request_rec* r);

/* Error logging */
int gate_log_error(request_rec* r, int level, const char* msg);

/*  Unit testing */
int gate_test_handler(request_rec *r);

END_DECL

#endif
