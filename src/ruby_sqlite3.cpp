#include <unistd.h>
#include <stdlib.h>

#include "ruby.hpp"

#include <sstream>

#include "ruby_sqlite3.h"
#include "ruby_sqlite3_stmt.h"

#define VALUE mrb_value

#define CLASS_NAME "SQLite3"

extern "C" {

static VALUE m_aborted(mrb_state* mrb, VALUE self);
static VALUE m_init(mrb_state* mrb, VALUE self);
static VALUE m_open(mrb_state* mrb, VALUE self);
static VALUE m_close(mrb_state* mrb, VALUE self);
static VALUE m_prepare(mrb_state* mrb, VALUE self);
static VALUE m_finalize(mrb_state* mrb, VALUE self);
static VALUE m_errno(mrb_state* mrb, VALUE self);
static VALUE m_error(mrb_state* mrb, VALUE self);
static VALUE m_begin(mrb_state* mrb, VALUE self);
static VALUE m_commit(mrb_state* mrb, VALUE self);
static VALUE m_rollback(mrb_state* mrb, VALUE self);
static VALUE m_exec(mrb_state* mrb, VALUE self);
static VALUE m_insert_id(mrb_state* mrb, VALUE self);

}

using std::stringstream;

struct connection
{
    sqlite3* handle;
    int rc;
    bool dont_delete;
};

static void deallocator(mrb_state* mrb, void* x)
{
    if(x != NULL)
    {
        connection* c = (connection*)x;

        if(c->handle != NULL)
        {
            if(c->dont_delete == false)
            {
                sqlite3_close(c->handle);
            }

            c->handle = NULL;
        }

        free(x);
    }
}

static const struct mrb_data_type ruby_sqlite_type = 
{
    CLASS_NAME, 
    deallocator
};

static struct RClass* cls;

connection* allocator(mrb_state* mrb)
{
    connection* c = (connection*)mrb_malloc(mrb, sizeof(connection));
    
    // Set dont_delete to false -- we have to clean this up as we allocated it.
    c->dont_delete == false;

    return c;
}

VALUE make_sqlite3_object(mrb_state* mrb, sqlite3* handle)
{
    connection* c = (connection*)malloc(sizeof(connection));

    // Store the handle
    c->handle = handle;

    // Set dont_delete to true -- we don't clean it up as we did not allocate
    // it.
    c->dont_delete == true;
    
    return mrb_obj_value(Data_Wrap_Struct(mrb, cls, &ruby_sqlite_type, c));
}

void init_sqlite3(mrb_state* mrb)
{
    cls = mrb_define_class(mrb, CLASS_NAME, mrb->object_class);
    MRB_SET_INSTANCE_TT(cls, MRB_TT_DATA);

    mrb_define_method(mrb, cls, "initialize",  (mrb_func_t)m_init, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "open",        (mrb_func_t)m_open, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "close",       (mrb_func_t)m_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "prepare",     (mrb_func_t)m_prepare, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "errno",       (mrb_func_t)m_errno, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "error",       (mrb_func_t)m_error, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "finalize",    (mrb_func_t)m_finalize, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "begin",       (mrb_func_t)m_begin, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "commit",      (mrb_func_t)m_commit, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "rollback",    (mrb_func_t)m_rollback, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "exec",        (mrb_func_t)m_exec, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "insertId",    (mrb_func_t)m_insert_id, MRB_ARGS_NONE());

    // SQLite constants
    mrb_define_global_const(mrb, "SQLITE_OK",         mrb_fixnum_value(SQLITE_OK));
    mrb_define_global_const(mrb, "SQLITE_ERROR",      mrb_fixnum_value(SQLITE_ERROR));
    mrb_define_global_const(mrb, "SQLITE_INTERNAL",   mrb_fixnum_value(SQLITE_INTERNAL));
    mrb_define_global_const(mrb, "SQLITE_PERM",       mrb_fixnum_value(SQLITE_PERM));
    mrb_define_global_const(mrb, "SQLITE_ABORT",      mrb_fixnum_value(SQLITE_ABORT));
    mrb_define_global_const(mrb, "SQLITE_BUSY",       mrb_fixnum_value(SQLITE_BUSY));
    mrb_define_global_const(mrb, "SQLITE_LOCKED",     mrb_fixnum_value(SQLITE_LOCKED));
    mrb_define_global_const(mrb, "SQLITE_NOMEM",      mrb_fixnum_value(SQLITE_NOMEM));
    mrb_define_global_const(mrb, "SQLITE_READONLY",   mrb_fixnum_value(SQLITE_READONLY));
    mrb_define_global_const(mrb, "SQLITE_INTERRUPT",  mrb_fixnum_value(SQLITE_INTERRUPT));
    mrb_define_global_const(mrb, "SQLITE_IOERR",      mrb_fixnum_value(SQLITE_IOERR));
    mrb_define_global_const(mrb, "SQLITE_CORRUPT",    mrb_fixnum_value(SQLITE_CORRUPT));
    mrb_define_global_const(mrb, "SQLITE_NOTFOUND",   mrb_fixnum_value(SQLITE_NOTFOUND));
    mrb_define_global_const(mrb, "SQLITE_FULL",       mrb_fixnum_value(SQLITE_FULL));
    mrb_define_global_const(mrb, "SQLITE_CANTOPEN",   mrb_fixnum_value(SQLITE_CANTOPEN));
    mrb_define_global_const(mrb, "SQLITE_PROTOCOL",   mrb_fixnum_value(SQLITE_PROTOCOL));
    mrb_define_global_const(mrb, "SQLITE_EMPTY",      mrb_fixnum_value(SQLITE_EMPTY));
    mrb_define_global_const(mrb, "SQLITE_SCHEMA",     mrb_fixnum_value(SQLITE_SCHEMA));
    mrb_define_global_const(mrb, "SQLITE_TOOBIG",     mrb_fixnum_value(SQLITE_TOOBIG));
    mrb_define_global_const(mrb, "SQLITE_CONSTRAINT", mrb_fixnum_value(SQLITE_CONSTRAINT));
    mrb_define_global_const(mrb, "SQLITE_MISMATCH",   mrb_fixnum_value(SQLITE_MISMATCH));
    mrb_define_global_const(mrb, "SQLITE_MISUSE",     mrb_fixnum_value(SQLITE_MISUSE));
    mrb_define_global_const(mrb, "SQLITE_NOLFS",      mrb_fixnum_value(SQLITE_NOLFS));
    mrb_define_global_const(mrb, "SQLITE_AUTH",       mrb_fixnum_value(SQLITE_AUTH));
    mrb_define_global_const(mrb, "SQLITE_ROW",        mrb_fixnum_value(SQLITE_ROW));
    mrb_define_global_const(mrb, "SQLITE_DONE",       mrb_fixnum_value(SQLITE_DONE));

    // SQLite storage classes
    mrb_define_global_const(mrb, "SQLITE_INTEGER",    mrb_fixnum_value(SQLITE_INTEGER));
    mrb_define_global_const(mrb, "SQLITE_FLOAT",      mrb_fixnum_value(SQLITE_FLOAT));
    mrb_define_global_const(mrb, "SQLITE_BLOB",       mrb_fixnum_value(SQLITE_BLOB));
    mrb_define_global_const(mrb, "SQLITE_NULL",       mrb_fixnum_value(SQLITE_NULL));
    mrb_define_global_const(mrb, "SQLITE_TEXT",       mrb_fixnum_value(SQLITE_TEXT));
}

VALUE m_init(mrb_state* mrb, VALUE self)
{
    connection* c;
    
    c = (connection*)DATA_PTR(self);
    
    if(c) 
    {
        deallocator(mrb, c);
    }
    
    DATA_TYPE(self) = &ruby_sqlite_type;
    DATA_PTR(self)  = NULL;
    c = allocator(mrb); 
    DATA_PTR(self) = c;
    
    return self;
}

RClass* sqlite3_class()
{
    return cls;
}

static connection* get_object(mrb_state* mrb, VALUE self)
{
    return DATA_CHECK_GET_PTR(mrb, self, &ruby_sqlite_type, connection);
}

#define ENSURE_CONNECTION(x)                                 \
    if(x->handle == NULL)                                    \
    {                                                        \
        stringstream msg;                                    \
        msg    << __FILE__ << ":" << __LINE__;               \
        mrb_raisef( mrb, E_RUNTIME_ERROR, "Invalid db handle: %s", \
                    msg.str().c_str());                            \
    }                                                    

sqlite3* get_sqlite3_handle(mrb_state* mrb, VALUE self)
{
    connection* c = DATA_CHECK_GET_PTR(mrb, self, &ruby_sqlite_type, connection);

    return c->handle;
}

VALUE m_errno(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    return mrb_fixnum_value(c->rc);
}

VALUE m_error(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    return mrb_str_new_cstr(mrb, sqlite3_errmsg(c->handle));
}

VALUE m_open(mrb_state* mrb, VALUE self)
{
    mrb_value db;
    mrb_get_args(mrb, "S", &db);

    connection* c = get_object(mrb, self);

    bool file_exists = (access(mrb_str_to_cstr(mrb, db), R_OK) == 0);

    c->rc = SQLITE_OK;
    c->rc = sqlite3_open(mrb_str_to_cstr(mrb, db), &c->handle);

    //register_sql_functions(c->handle);

    // If the file already exists
    if(file_exists)
    {
        // sqlite3_open() will open anything and not complain. It does not check to
        // see if the file is a valid SQLite database. We need to know this now. So
        // we check to see if this is in fact a valid SQLite database file by
        // compiling a simple statement. The compilation alone will force SQLite to
        // go the the file for inforamtion, which will determine its validity. If
        // this statement does not compile, we don't have a valid database.
        
        const char* sql = "select count(*) from sqlite_master";
        sqlite3_stmt* stmt;
        c->rc = sqlite3_prepare(c->handle, sql, strlen(sql), &stmt, NULL);
        sqlite3_finalize(stmt);
    }
        
    return mrb_fixnum_value(c->rc);
}

VALUE m_close(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle != NULL)
    {
        c->rc = sqlite3_close(c->handle);

        // If rc != SQLITE_OK, then there is an unfinalized statement, and the
        // connection was not closed. We return c->rc to indicate this, so
        // caller at least can check for this condition.
        if(c->rc == SQLITE_OK)
        {
            // Closed. Set handle to NULL to indiciate this
            c->handle = NULL;
        }
    }
    else
    {
        c->rc = SQLITE_OK;
    }

    return  mrb_fixnum_value(c->rc);
}

VALUE m_prepare(mrb_state* mrb, VALUE self)
{
    mrb_value sql;
    mrb_get_args(mrb, "S", &sql);
    mrb_check_type(mrb, sql, MRB_TT_STRING);

    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle == NULL)
    {
        c->rc = SQLITE_ERROR;

        return mrb_nil_value();
    }

    return make_sqlite3_stmt(mrb, self, c->handle, sql);
}

struct statement
{
    sqlite3_stmt* handle;
};

VALUE m_finalize(mrb_state* mrb, VALUE self)
{
    mrb_value stmt_object;
    mrb_get_args(mrb, "S", &stmt_object);

    // Require that db be a C ext type
    mrb_check_type(mrb, stmt_object, MRB_TT_DATA);

    // Require that db be of Gintana::SQLite class
    if(mrb_obj_is_instance_of(mrb, stmt_object, sqlite3_stmt_class()) == MRB_TT_FALSE)
    {
        mrb_raisef( mrb, E_RUNTIME_ERROR, 
                    "wrong argument type %s (expected %s)",
                    mrb_obj_classname(mrb, stmt_object),
                    mrb_class_name(mrb, cls)
            );
    }

    connection* c = get_object(mrb, self);
    c->rc = SQLITE_OK;

    statement* stmt = DATA_CHECK_GET_PTR(mrb, stmt_object, &ruby_sqlite_statement_type, statement);
    // Data_Get_Struct(mrb, stmt_object, statement, stmt);

    c->rc = sqlite3_finalize(stmt->handle);

    // Set to NULL so it won't be finalized by destructor
    stmt->handle = NULL;

    return mrb_fixnum_value(c->rc);
}

VALUE m_begin(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle == NULL)
    {
        c->rc = SQLITE_ERROR;

        return mrb_nil_value();
    }

    c->rc = sqlite3_exec(c->handle, "BEGIN", NULL, NULL, NULL);

    return  mrb_fixnum_value(c->rc);
}

VALUE m_commit(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle == NULL)
    {
        c->rc = SQLITE_ERROR;

        return mrb_nil_value();
    }

    c->rc = sqlite3_exec(c->handle, "COMMIT", NULL, NULL, NULL);

    return  mrb_fixnum_value(c->rc);
}

VALUE m_rollback(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle == NULL)
    {
        c->rc = SQLITE_ERROR;

        return mrb_nil_value();
    }

    c->rc = sqlite3_exec(c->handle, "ROLLBACK", NULL, NULL, NULL);

    return  mrb_fixnum_value(c->rc);
}

VALUE m_exec(mrb_state* mrb, VALUE self)
{
    mrb_value sql;
    mrb_get_args(mrb, "S", &sql);
    mrb_check_type(mrb, sql, MRB_TT_STRING);

    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    if(c->handle == NULL)
    {
        c->rc = SQLITE_ERROR;

        return mrb_nil_value();
    }

    mrb_check_type(mrb, sql, MRB_TT_STRING);

    const char* zSQL = mrb_str_to_cstr(mrb, sql);

    c->rc = sqlite3_exec(c->handle, zSQL, NULL, NULL, NULL);

    return mrb_fixnum_value(c->rc);
}

VALUE m_insert_id(mrb_state* mrb, VALUE self)
{
    connection* c = get_object(mrb, self);

    ENSURE_CONNECTION(c)

    return mrb_fixnum_value(sqlite3_last_insert_rowid(c->handle));
}
