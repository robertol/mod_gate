#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_log.h>

#include <ap_config.h>

#include "common.h"
#include "module.h"

module AP_MODULE_DECLARE_DATA gate_module;

/* Module Interface */

/* Child shutdown handler */
static apr_status_t gate_child_shutdown(void* data)
{
    gate_shutdown_module();

    return APR_SUCCESS;
}

/* Child init hook. This will be called exactly once. */
static void gate_child_init_hook(apr_pool_t* child_pool, server_rec* s)
{
    gate_init_module(child_pool, s);

    /* Have the gate_child_shutdown called when child_pool is destroyed. Thus,
    ** when the process shuts down, we shut down Gate as well. 
    */
    apr_pool_cleanup_register( child_pool, NULL, 
                               gate_child_shutdown, 
                               apr_pool_cleanup_null );
}

/* Gate handler */
static int gate_handler(request_rec *r)
{
    return gate_request_handler(r);
}

static void register_hooks(apr_pool_t *p)
{
    ap_hook_child_init(gate_child_init_hook, NULL, NULL, APR_HOOK_MIDDLE);

    /* The generic handler */
    ap_hook_handler(gate_handler, NULL, NULL, APR_HOOK_FIRST);
}

static const char* 
set_gate_default_database(cmd_parms *parms, void* config, const char* arg)
{
    if(arg == NULL)
    {
        return NULL;
    }
    
    // If this is in a server config (outside directory)
    if(ap_check_cmd_context(parms, NOT_IN_DIR_LOC_FILE) == NULL)
    {
        gate_config* cfg = ap_get_module_config( parms->server->module_config,
                                                &gate_module );
        
        cfg->default_database = (char*)arg;
        
        return NULL;
    }

    gate_dir_config* dir_config = (gate_dir_config*)config;
    apr_table_set(dir_config->options, "GateDatabase", (char*)arg);
    
    /* Success */
    return NULL;
}

static const char* 
set_gate_config_var( cmd_parms *parms, 
                    void* config, 
                    const char* arg1,
                    const char* arg2 )
{
    if(arg1 == NULL)
    {
        return NULL;
    }
    
    // If this is in a server config (outside directory)
    if(ap_check_cmd_context(parms, NOT_IN_DIR_LOC_FILE) == NULL)
    {
        gate_config* cfg = ap_get_module_config( parms->server->module_config,
                                                &gate_module );
        
        apr_table_set(cfg->options, (char*)arg1, (char*)arg2);
        
        return NULL;
    }
    
    gate_dir_config* dir_config = (gate_dir_config*)config;
    apr_table_set(dir_config->options, (char*)arg1, (char*)arg2);
    
    /* Success */
    return NULL;
}

static const char* 
set_gate_env_var( cmd_parms* parms, 
                  void* config, 
                  const char* arg1,
                  const char* arg2 )
{
    if(arg1 == NULL)
    {
        return NULL;
    }

    if(arg2 == NULL)
    {
        return NULL;
    }
    
    setenv(arg1, arg2, 1);
        
    /* Success */
    return NULL;
}

/* ---------------------------------------------------------------------------- */
/* Custom Handlers */
/* ---------------------------------------------------------------------------- */

/* Remember that the void* config parameter in the set_xxx commands always
** points to a dir_config. If you need the server_config, you have to use
** ap_get_module_config. 
**
** Furthermore, NEVER use apr_pstrdup() when assigning values from *arg to a
** server_config struct. As a rule, avoid apr_pstrdup() altogther in
** configuration phase.
*/

static const char* 
set_gate_filter(cmd_parms *parms, void* config, const char* arg)
{
    if(arg == NULL)
    {
        return NULL;
    }
    
    // If this is in a server config (outside directory). This can only be in
    // either global environment or <VirtualHost>.
    if(ap_check_cmd_context(parms, NOT_IN_DIR_LOC_FILE) == NULL)
    {
        gate_config* cfg = ap_get_module_config( parms->server->module_config,
                                                 &gate_module );
        
        /* DO NOT use apr_pstrdup(parms->pool, arg) here to make a copy of the
        ** string before assignbment. It will cause a segfault.
        */
        cfg->handler = arg;
        
        return NULL;
    }
    
    gate_dir_config* dir_config = (gate_dir_config*)config;
    apr_table_set(dir_config->options, "Gate", (char*)arg);

    /* Success */
    return NULL;
}

static const char* 
set_gate_filter_declare(cmd_parms *parms, void* config, const char* arg)
{
    if(arg == NULL)
    {
        return NULL;
    }
    
    const char* errmsg = ap_check_cmd_context(parms, NOT_IN_DIR_LOC_FILE);

    // If this is outside of a directory
    if(errmsg == NULL)
    {
        gate_config* cfg = ap_get_module_config( parms->server->module_config,
                                                &gate_module );

        // Check to see if this handler has been registered
        apr_hash_t* handler = apr_hash_get( cfg->handlers, (char*)arg, 
                                            APR_HASH_KEY_STRING);

        // If it doesn't exist
        if(handler == NULL)
        {            
            // Create it
            apr_hash_t* h_config = apr_hash_make(parms->pool);
            apr_hash_set( cfg->handlers,
                          (char*)arg, APR_HASH_KEY_STRING, (void*)h_config);
        }
    }
        
    return errmsg;
}


static const char* 
gate_filter_set_params( cmd_parms *parms, const char* key,
                        const char* handler, const char* value )
{
    if((handler == NULL) || (value == NULL)) 
    {
        return apr_psprintf( parms->temp_pool, 
                             "Handler %s %s arguments not provided",
                             handler, value );
    }

    const char* errmsg = ap_check_cmd_context(parms, NOT_IN_DIR_LOC_FILE);

    // If this is outside of a directory
    if(errmsg != NULL)
    {
        return errmsg;
    }

    gate_config* cfg = ap_get_module_config( parms->server->module_config,
                                            &gate_module );
        
    // Check to see if this handler has been registered
    apr_hash_t* h_config = apr_hash_get( cfg->handlers, (char*)handler, 
                                         APR_HASH_KEY_STRING);

    // If it doesn't exist
    if(h_config == NULL)
    {
        return apr_psprintf( parms->temp_pool, 
                             "Handler %s is not registered",
                             handler);
    }
   
    // Set parameters
    apr_hash_set( h_config, (char*)key, APR_HASH_KEY_STRING, value);

    return NULL;
}

static const char* 
set_gate_filter_config( cmd_parms *parms, void* config, 
                        const char* handler, 
                        const char* key, const char* value )
{
    return gate_filter_set_params( parms, key, handler, value );
}

static const char* 
set_gate_filter_database( cmd_parms *parms, void* config, 
                          const char* arg1, const char* arg2 )
{
    return gate_filter_set_params( parms, 
                                   "GateDatabase",
                                   arg1, arg2 );
}

static const command_rec mod_gate_cmds[] =
{
    AP_INIT_TAKE1(
        "GateDefaultDatabase",
        set_gate_default_database,
        NULL,
        RSRC_CONF | OR_ALL,
        "GateDatabase <string> "
        "-- set default Gate module for Apache handler."
        ),
            
    AP_INIT_TAKE1(
        "Gate",
        set_gate_filter,
        NULL,
        RSRC_CONF | OR_ALL,
        "Gate {name} "
        "-- set a Gate handler."
        ),

    AP_INIT_TAKE2(
        "GateConfig",
        set_gate_config_var,
        NULL,
        RSRC_CONF | OR_ALL,
        "GateConfig {key} {value} "
        "-- set server/per-directory variable for Apache handler."
    ),

    AP_INIT_TAKE2(
        "GateEnv",
        set_gate_env_var,
        NULL,
        RSRC_CONF | OR_ALL,
        "GateEnv {key} {value} "
        "-- set ENV variable for Apache handler."
    ),

    AP_INIT_TAKE1(
        "GateDeclare",
        set_gate_filter_declare,
        NULL,
        RSRC_CONF,
        "GateDeclare {name} "
        "-- declare a custom Gate handler."
    ),

    AP_INIT_TAKE2(
        "GateDatabase",
        set_gate_filter_database,
        NULL,
        RSRC_CONF,
        "GateDatabase {handler} {db_path} "
        "-- set Gate database for handler."
    ),

    AP_INIT_TAKE3(
        "GateConfig",
        set_gate_filter_config,
        NULL,
        RSRC_CONF,
        "GateConfig {handler} {key} {value}"
        " -- define key/value config setting for handler."
    ),

    {NULL}
};

static void* create_config(apr_pool_t* p, server_rec *s)
{
    gate_config* cfg;

    /* allocate space for the configuration structure from the provided pool p. */
    cfg = (gate_config*)apr_pcalloc(p, sizeof(gate_config));

    cfg->handler          = NULL;
    cfg->default_database = NULL;
    
    cfg->options    = apr_table_make(p, 3);
    cfg->handlers   = apr_hash_make(p);

    /* return the new server configuration structure. */
    return (void*)cfg;
}

static void* create_dir_conf(apr_pool_t* p, char* dir)
{
    gate_dir_config* cfg = apr_pcalloc(p, sizeof(gate_dir_config));

    cfg->options = apr_table_make(p, 3);
    cfg->dir     = dir;

    /* Debugging
    ap_log_perror( APLOG_MARK, APLOG_WARNING, 
                   0, p, "create_dir_conf %x->%s", 
                   cfg, dir);
    */

    return cfg;
}

static void* merge_dir_conf(apr_pool_t* pool, void* current_config, void* new_config)
{
    gate_dir_config* dir_current = (gate_dir_config*)current_config;
    gate_dir_config* dir_new     = (gate_dir_config*)new_config;
    gate_dir_config* dir_merged  = apr_palloc(pool, sizeof(gate_dir_config));
    
    dir_merged->options = apr_table_overlay( pool, 
                                             dir_current->options, 
                                             dir_new->options );

    apr_table_compress(dir_merged->options, APR_OVERLAP_TABLES_SET);

    /* Debugging
    const char* handler = apr_table_get(dir_merged->options, "Gate");

    ap_log_perror(APLOG_MARK, APLOG_WARNING, 
                  0, pool, "Dir %x %s->%s Handler: %s ", 
                  dir_merged, dir_current->dir, dir_new->dir, handler);
    */

    /* This will cause a segfault (by using strdup)
       dir_merged->dir = apr_pstrdup(pool, dir_new->dir);

       > gdb --args /usr/sbin/apache2 -f 
       > /etc/apache2/httpd.conf -DONE_PROCESS -DNO_DETACH 
    */

    // This will not
    dir_merged->dir = dir_new->dir;

    return dir_merged;
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA gate_module = {
    STANDARD20_MODULE_STUFF, 
    create_dir_conf,   /* create per-dir config    */
    merge_dir_conf,    /* merge per-dir config     */
    create_config,     /* create per-server config */
    NULL,              /* merge per-server config  */
    mod_gate_cmds,     /* config file cmds         */
    register_hooks     /* register hooks           */
};

module AP_MODULE_DECLARE_DATA _module = {
    STANDARD20_MODULE_STUFF, 
    create_dir_conf,   /* create per-dir config    */
    merge_dir_conf,    /* merge per-dir config     */
    create_config,     /* create per-server config */
    NULL,              /* merge per-server config  */
    mod_gate_cmds,     /* config file cmds         */
    register_hooks     /* register hooks           */
};
