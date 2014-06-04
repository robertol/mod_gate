#ifndef MODJERK_MODULE_DECL
#define MODJERK_MODULE_DECL

#include <http_protocol.h>
#include <http_config.h>

#include "common.h"

#include <apr_hash.h>

BEGIN_DECL

/** 
This structure holds the configuration parameters in the Apache module.
*/
typedef struct jerk_config
{
    char* handler;                  ///< Current handler selected for given request
    char* default_database; ///< Default handler database

    apr_table_t* options;          ///< Apache handler user-defined options to pass in
    apr_hash_t* handlers;          ///< User-defined Apache handlers 

} jerk_config;

/** 
This structure holds directory configuration parameters in the JERK Apache module.
*/
typedef struct jerk_dir_config
{
    char* dir;
    apr_table_t* options;
} jerk_dir_config;

extern module AP_MODULE_DECLARE_DATA jerk_module;

jerk_config* jerk_server_config(ap_conf_vector_t* module_config);
jerk_dir_config* jerk_directory_config(request_rec* r);
apr_hash_t* jerk_handler_config(request_rec* r, const char* name);

/* Main module functions */
int jerk_init_module(apr_pool_t* p, server_rec* s);
int jerk_shutdown_module();
int jerk_request_handler(request_rec* req);
int jerk_request_init_configuration(request_rec* r);

/* Error logging */
int jerk_log_error(request_rec* r, int level, const char* msg);

/*  Unit testing */
int jerk_test_handler(request_rec *r);

END_DECL

#endif
