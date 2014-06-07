#include <sstream>
#include <string>

#include "ruby.hpp"

#include "ruby_sqlite3.h"
#include "ruby_sqlite3_stmt.h"

#define VALUE mrb_value

typedef VALUE (*fn)(...);

#define CLASS_NAME "SQLite3Stmt"

using std::string;
using std::stringstream;

extern "C" {

static VALUE m_init(mrb_state* mrb, VALUE self);
static VALUE m_step(mrb_state* mrb, VALUE self);
static VALUE m_keys(mrb_state* mrb, VALUE self);
static VALUE m_each(mrb_state* mrb, VALUE self);
static VALUE m_columns(mrb_state* mrb, VALUE self);
static VALUE m_column_type(mrb_state* mrb, VALUE self);
static VALUE m_column(mrb_state* mrb, VALUE self);
static VALUE m_column_text(mrb_state* mrb, VALUE self);
static VALUE m_column_int(mrb_state* mrb, VALUE self);
static VALUE m_column_double(mrb_state* mrb, VALUE self);

}

struct statement
{
    sqlite3_stmt* handle;
};

static void deallocator(mrb_state* mrb, void* x)
{
    statement* stmt = (statement*)x;

    if(stmt != NULL)
    {
        if(stmt->handle != NULL)
        {
            sqlite3_finalize(stmt->handle);
            
            stmt->handle = NULL;
        }

        mrb_free(mrb, stmt);
    }
}

const struct mrb_data_type ruby_sqlite_statement_type = 
{
    CLASS_NAME, 
    deallocator
};

static statement* allocator(VALUE cls)
{
    return NULL;
}

static struct RClass* cls;

void init_sqlite3_stmt(mrb_state* mrb)
{
    cls = mrb_define_class(mrb, CLASS_NAME, mrb->object_class);
    MRB_SET_INSTANCE_TT(cls, MRB_TT_DATA);

    mrb_define_method(mrb, cls, "initialize",   (mrb_func_t)m_init, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "step",         (mrb_func_t)m_step, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "each",         (mrb_func_t)m_each, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "keys",         (mrb_func_t)m_keys, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "columns",      (mrb_func_t)m_columns, MRB_ARGS_NONE());
    mrb_define_method(mrb, cls, "columnType",   (mrb_func_t)m_columns, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "column",       (mrb_func_t)m_column, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "[]",           (mrb_func_t)m_column, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "columnText",   (mrb_func_t)m_column_text, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "columnInt",    (mrb_func_t)m_column_int, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cls, "columnDouble", (mrb_func_t)m_column_double, MRB_ARGS_REQ(1));
}

VALUE m_init(mrb_state* mrb, VALUE self)
{
    mrb_raise( mrb, E_RUNTIME_ERROR, 
               "Cannot create a Apache::Server object this way" );
}

VALUE make_sqlite3_stmt(mrb_state* mrb, VALUE connection, sqlite3* db, VALUE sql)
{
    mrb_check_type(mrb, sql, MRB_TT_STRING);

    // Require that db be of Gintana::SQLite class
    if(mrb_obj_is_instance_of(mrb, connection, sqlite3_class()) == MRB_TT_FALSE)
    {
        mrb_raisef(mrb, E_RUNTIME_ERROR, 
                   "wrong argument type %s (expected %s)",
                   mrb_obj_classname(mrb, connection),
                   mrb_class_name(mrb, cls)
            );
    }

    const char* zSQL = mrb_str_to_cstr(mrb, sql);

    // Allocate statement struct
    statement* stmt = (statement*)mrb_malloc(mrb, sizeof(statement));

    int rc = sqlite3_prepare(db, zSQL, strlen(zSQL), &stmt->handle, NULL);

    if(rc != SQLITE_OK)
    {
        // Free allocated memory
        mrb_free(mrb, stmt);
        
        mrb_raisef( mrb, E_RUNTIME_ERROR, "sqlite::prepare() - %s", 
                    sqlite3_errmsg(db) );
    }

    // Create Ruby stmt object
    VALUE obj = mrb_obj_value(Data_Wrap_Struct(mrb, cls, &ruby_sqlite_statement_type, stmt));

    // Assign the sqlite3 instance to keep a reference count on it.
    mrb_iv_set(mrb, obj, mrb_intern_cstr(mrb, "connection"), connection);

    return obj;
}

RClass* sqlite3_stmt_class()
{
    return cls;
}

statement* get_object(mrb_state* mrb, VALUE self)
{
    return DATA_CHECK_GET_PTR(mrb, self, &ruby_sqlite_statement_type, statement);
}

#define ENSURE_STATEMENT_HANDLE(x)                                        \
    if(x == NULL)                                                   \
    {                                                               \
        stringstream msg;                                           \
        msg    << __FILE__ << ":" << __LINE__;                      \
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Invalid statement handle: %s", \
                   msg.str().c_str());                                  \
    }                                                    

VALUE m_step(mrb_state* mrb, VALUE self)
{
    statement* stmt = get_object(mrb, self);

    ENSURE_STATEMENT_HANDLE(stmt->handle)

    return mrb_fixnum_value(sqlite3_step(stmt->handle));
}

VALUE m_columns(mrb_state* mrb, VALUE self)
{
    statement* stmt = get_object(mrb, self);

    ENSURE_STATEMENT_HANDLE(stmt->handle)
    
    return mrb_fixnum_value(sqlite3_column_count(stmt->handle));
}

VALUE m_column_type(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);
    mrb_check_type(mrb, ordinal, MRB_TT_FIXNUM);

    statement* stmt = get_object(mrb, self);
    
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    return mrb_fixnum_value(sqlite3_column_type(stmt->handle, mrb_fixnum(ordinal)));
}

VALUE column_value(mrb_state* mrb, statement* stmt, int i)
{
    int type = sqlite3_column_type(stmt->handle, i);

    switch(type)
    {
        case SQLITE_INTEGER:
        {
            return mrb_fixnum_value(sqlite3_column_int(stmt->handle, i));
        }

        case SQLITE_FLOAT:
        {
            return mrb_float_value(mrb, sqlite3_column_double(stmt->handle, i));
        }

        case SQLITE_BLOB:
        {
            return mrb_str_new( mrb, 
                                (const char*)sqlite3_column_blob(stmt->handle, i), 
                                sqlite3_column_bytes(stmt->handle, i) );
        }

        case SQLITE_NULL:
        {
            return mrb_nil_value();
        }
        
        case SQLITE_TEXT:
        {
            return mrb_str_new( mrb, 
                                (const char*)sqlite3_column_text(stmt->handle, i), 
                                sqlite3_column_bytes(stmt->handle, i) );
        }
    }

    return mrb_nil_value();
}

VALUE m_column(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);

    statement* stmt = get_object(mrb, self);
    
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int i = mrb_fixnum(ordinal);

    return column_value(mrb, stmt, i);
}

VALUE m_each(mrb_state* mrb, VALUE self)
{
    mrb_value blk;
    mrb_get_args(mrb, "&", &blk);

    statement* stmt = get_object(mrb, self);

    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int cols = sqlite3_column_count(stmt->handle);

    int i;
    for(i=0; i<cols; i++)
    {
        mrb_value array = mrb_ary_new(mrb);
        mrb_ary_push(mrb, array, mrb_str_new_cstr(mrb, sqlite3_column_name(stmt->handle, i)));
        mrb_ary_push(mrb, array, column_value(mrb, stmt, i));

        mrb_yield(mrb, blk, array);
    }

    return mrb_nil_value();
}

VALUE m_keys(mrb_state* mrb, VALUE self)
{
    statement* stmt = get_object(mrb, self);

    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int cols = sqlite3_column_count(stmt->handle);
    
    VALUE a = mrb_ary_new(mrb);

    int i;
    for(i=0; i<cols; i++)
    {
        mrb_ary_push(mrb, a, mrb_str_new_cstr(mrb, sqlite3_column_name(stmt->handle,i)));
    }
    
    return a;
}

VALUE m_column_text(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);

    statement* stmt = get_object(mrb, self);
    
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int i = mrb_fixnum(ordinal);

    // Check that ordinal is within bounds
    int cols = sqlite3_column_count(stmt->handle);

    if(i >= cols)
    {
        return mrb_nil_value();
    }

    int type = sqlite3_column_type(stmt->handle, i);

    return mrb_str_new( mrb, 
                        (const char*)sqlite3_column_text(stmt->handle, i), 
                        sqlite3_column_bytes(stmt->handle, i) );
}

VALUE m_column_blob(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);

    statement* stmt = get_object(mrb, self);
    
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int i = mrb_fixnum(ordinal);

    // Check that ordinal is within bounds
    int cols = sqlite3_column_count(stmt->handle);

    if(i >= cols)
    {
        return mrb_nil_value();
    }

    int type = sqlite3_column_type(stmt->handle, i);

    return mrb_str_new( mrb,
                        (const char*)sqlite3_column_blob(stmt->handle, i), 
                        sqlite3_column_bytes(stmt->handle, i) );
}

VALUE m_column_int(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);

    statement* stmt = get_object(mrb, self);
   
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int i = mrb_fixnum(ordinal);

    // Check that ordinal is within bounds
    int cols = sqlite3_column_count(stmt->handle);

    if(i >= cols)
    {
        return mrb_nil_value();
    }

    int type = sqlite3_column_type(stmt->handle, i);

    return mrb_fixnum_value(sqlite3_column_int(stmt->handle, i));
}

VALUE m_column_double(mrb_state* mrb, VALUE self)
{
    mrb_value ordinal;
    mrb_get_args(mrb, "i", &ordinal);

    statement* stmt = get_object(mrb, self);
    
    ENSURE_STATEMENT_HANDLE(stmt->handle)

    int i = mrb_fixnum(ordinal);

    // Check that ordinal is within bounds
    int cols = sqlite3_column_count(stmt->handle);

    if(i >= cols)
    {
        return mrb_nil_value();
    }

    int type = sqlite3_column_type(stmt->handle, i);

    return mrb_float_value(mrb, sqlite3_column_double(stmt->handle, i));
}
