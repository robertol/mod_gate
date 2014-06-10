#include <stdarg.h>

#include "ruby.hpp"

#include <sstream>

using std::string;

namespace mruby {

Object::Object(mrb_state* vm, const char* name, int n, ...)
    : mrb(vm), self(mrb_nil_value()), _class_name()
{
    va_list vl;
    va_start(vl, n);
    
    /*
    for(int i=0;i<count;i++)
    {
    
    }
    */

    self = mruby::create_object(mrb, name, n, vl);

    va_end(vl);

    mruby::register_object(mrb, self);

    _class_name = name;
}

Object::~Object()
{
    mruby::free_object(mrb, self);
}

const char* Object::class_name()
{
    return _class_name.c_str();
}

VALUE Object::method(const char* name, int n, ...)
{
    VALUE *argv = 0;

    if (n > 0) 
    {
        argv = (VALUE*)mrb_alloca(mrb, sizeof(VALUE)*n);
        va_list ar;
        va_start(ar, n);

        int i;
        for(i=0; i<n ;i++)
        {
            argv[i] = va_arg(ar, VALUE);
        }

        va_end(ar);
    } 

    Arguments arg;
    arg.recv = self;
    arg.id   = mrb_intern_cstr(mrb, name);
    arg.n    = n;
    arg.argv = argv;

    /*
    VALUE method_wrap(VALUE arg)
    {
        Arguments &a = *reinterpret_cast<Arguments*>(arg);
        
        return rb_funcall2(a.recv, a.id, a.n, a.argv);
    }
    */

    int error = 0;
    //VALUE result = mrb_protect(mruby::method_wrap, reinterpret_cast<VALUE>(&arg), &error);
    value result = rb_funcall(mrb, arg.recv, arg.id, a.n, a.argv);

    if(mrb->exc)
    {
        std::string msg;
        msg = "mruby::Object::method() invoking " 
              + _class_name + (std::string)"::" 
              + name + (std::string)"()" ;
        
        Exception e(msg.c_str());
        e.backtrace();

        throw e;
    }

    return result;
}

//------------------------------------------------------------------------------
// Memory Management 
//------------------------------------------------------------------------------

Objects* Objects::singleton = 0;
VALUE Objects::objects;

Objects* Objects::instance()
{
    if(singleton == NULL)
    {
        singleton = new Objects();
    }

    return singleton;
}

Objects::Objects(mrb_state* vm) : mrb(vm)
{
    objects = mrb_ary_new(mrb);
    rb_gc_register_address(mrb, &objects);
}

Objects::~Objects()
{
    rb_gc_unregister_address(mrb, &objects);
}

void Objects::register_object(VALUE object)
{
    rb_ary_push(mrb, objects, object);
}

void Objects::free_object(VALUE object)
{
    rb_ary_delete(mrb, objects, object);
}

void Objects::free_all()
{
    if(singleton != NULL)
    {
        delete singleton;
        singleton = NULL;
    }
}

void register_object(VALUE object)
{
    Objects::instance()->register_object(object);
}

void free_object(VALUE object)
{
    Objects::instance()->free_object(object);
}

void free_all()
{
    Objects::free_all();
}

//------------------------------------------------------------------------------
// Exceptions
//------------------------------------------------------------------------------

Exception::Exception(const char* msg) : _msg(), _backtrace(), _type()
{
    if(msg != NULL)
    {
        _msg = msg;
    }    
}

Exception::Exception(const Exception& e) : _msg(), _backtrace(), _type()
{
    *this = e;
}

Exception::~Exception() throw()
{

}

const Exception& Exception::operator=(const Exception& e)
{
    if(this != &e)
    {
        _msg       = e._msg;
        _backtrace = e._backtrace;
        _type      = e._type;
    }

    return *this;
}

const char* Exception::type() const
{
    return _type.c_str();
}

const char* Exception::what() const throw()
{
    return _msg.c_str();
}

const char* Exception::stackdump() const
{
    return _backtrace.c_str();
}

// Convert a ruby exception into C++. 
void Exception::backtrace() throw()
{
    _backtrace.clear();    
}

//------------------------------------------------------------------------------
// Function Calls
//------------------------------------------------------------------------------

extern "C" {
static int collect_hash_vals(VALUE key, VALUE value, VALUE data);
}

static int collect_hash_vals(VALUE key, VALUE value, VALUE data)
{
    std::map<string, string>* values = (std::map<string, string>*)data;

    std::stringstream strm;
    
    string key_str;

    switch(TYPE(key))
    {
        case T_STRING:
        {
            strm << StringValuePtr(key);
            break;
        }
        
        case T_FIXNUM:
        {
            strm << NUM2INT(key);
        }

        default:
        {

        }
    }

    key_str = strm.str();

    strm.str(std::string());

    string value_str;

    switch(TYPE(value))
    {
        case T_STRING:
        {
            strm << StringValuePtr(value);
            break;
        }
        
        case T_FIXNUM:
        {
            strm << NUM2INT(value);
        }

        default:
        {

        }
    }

    value_str = strm.str();

    (*values)[key_str] = value_str;

    return ST_CONTINUE;
}

typedef int (*iterfn)(...);

bool copy_hash(VALUE hash, std::map<std::string, std::string>& values)
{
    rb_hash_foreach(hash, (iterfn)collect_hash_vals, (VALUE)&values);

    return true;
}

/* Purpose: Call a ruby function in a safe way. Translate ruby errors into c++
** exceptions.
**
**    VALUE Unsafe() {
**        return rb_funcall(
**            self, 
**            rb_intern("test"), 
**            1, 
**            INT2NUM(42)
**        );
**    }
**
**    VALUE Safe() {
**        return mruby::method(
**            self, 
**            rb_intern("test"), 
**            1, 
**            INT2NUM(42)
**        );
**    }
*/

VALUE method(mrb_state* mrb, VALUE recv, ID id, int n, ...)
{
    VALUE *argv = 0;

    if (n > 0) 
    {
        argv = ALLOCA_N(VALUE, n);
        va_list ar;
        va_start(ar, n);

        int i;
        for(i=0; i<n ;i++)
        {
            argv[i] = va_arg(ar, VALUE);
        }

        va_end(ar);
    } 

    Arguments arg;
    arg.recv = recv;
    arg.id   = id;
    arg.n    = n;
    arg.argv = argv;

    int error = 0;
    VALUE result = mrb_protect(mrb, method_wrap, reinterpret_cast<VALUE>(&arg), &error);

    if(error)
    {
        Exception e;
        e.backtrace();
        throw e;
    }

    return result;
}

VALUE vm_method(mrb_state* mrb, VALUE recv, ID id, int n, va_list ar)
{
    VALUE *argv = 0;

    if (n > 0) 
    {
        argv = ALLOCA_N(VALUE, n);

        int i;
        for(i=0; i<n ;i++)
        {
            argv[i] = va_arg(ar, VALUE);
        }
    } 

    Arguments arg;
    arg.recv = recv;
    arg.id   = id;
    arg.n    = n;
    arg.argv = argv;

    int error = 0;
    VALUE result = rb_protect(method_wrap, reinterpret_cast<VALUE>(&arg), &error);

    if(error)
    {
        Exception e;
        e.backtrace();
        throw e;
    }

    return result;
}

//------------------------------------------------------------------------------
// Module and Class Instantiation
//------------------------------------------------------------------------------

void eval(mrb_state* mrb, const char* code, const char* filename, int sl, VALUE binding)
{
    const char* fn = filename;

    if(fn == NULL)
    {
        fn = "eval";
    }
    
    // We use this rather than mruby::eval() because this allows us
    // to associated a file name with the eval code, giving us more
    // informative backtraces. Otherwise, mruby::eval() would simply
    // list the source of every frame as "(eval)".
    
    method( mrb_nil_value(), rb_intern("eval"), 4,
            rb_str_new2(code),     // code
            binding,               // binding
            rb_str_new2(fn),       // filename
            INT2NUM(sl) );         // source line
    
    /* old method

    // Set the Ruby source line to 0. This is a Ruby hack. There might be a
    // better way to do this, but we are telling Ruby that we are starting a new
    // file and that the first line of this chunk of code is line 1.

    ruby_sourceline = sl;

    rb_eval_string_protect(code, &error);

    if(error)
    {
        linterra::buffer msg;

        throw Exception();
    }
    */
}

VALUE require_protect(mrb_state* mrb, VALUE arg)
{
    const char *filename = reinterpret_cast<const char*>(arg);
    rb_require(filename);

    return mrb_nil_value();
}

bool call_function(mrb_state* mrb, const char* method, int n, ...)
{
    VALUE ret;
    va_list ar;

    if(n > 0) 
    {
        va_start(ar, n);
    }

    try
    {  
        ret = mruby::vm_method(mrb_nil_value(), rb_intern(method), n, ar);
    }
    catch(const ::mruby::Exception &e)
    {
        // User needs to see this error on command line, so will we pipe it to
        // stdout.
        fprintf(stdout, "%s\n", e.what());
        
        ret = Qfalse;
    }

    if(n > 0) 
    {
        va_end(ar);
    } 

    return ret;
}

void require(mrb_state* mrb, const char* filename)
{
    int error = 0;
    rb_protect(require_protect, reinterpret_cast<VALUE>(filename), &error);

    if(error)
    {
        std::stringstream strm;

        strm << "error loading " << filename << ".rb";

        Exception e(strm.str().c_str());
        e.backtrace();
        throw e;
    }
}

void load(mrb_state* mrb, const char* filename, int anonymous)
{
    int error = 0;
    rb_load_protect(rb_str_new2(filename), anonymous, &error);

    if(error)
    {
        Exception e;
        e.backtrace();
        throw e;
    }
}

struct NewArguments
{
    const char* class_name;
    int n;
    VALUE* argv;

    NewArguments( const char* cname, 
                  int n, 
                  VALUE* argv) 
        : class_name(cname), n(n), argv(argv)
    {

    }
};

VALUE create_object_protect(mrb_state* mrb, VALUE arg)
{
    NewArguments &a = *reinterpret_cast<NewArguments*>(arg);
    //VALUE class_name = rb_const_get(rb_cObject, rb_intern(a.class_name));
    VALUE class_name = rb_path2class(a.class_name);

    VALUE self = rb_class_new_instance(a.n, a.argv, class_name);
    //VALUE self = rb_funcall2(class_name, rb_intern("new"), a.n, a.argv);

    return self;
}

VALUE create_object(mrb_state* mrb, const char* class_name, int n, va_list ar)
{
    VALUE *argv = 0;

    if (n > 0) 
    {
        argv = ALLOCA_N(VALUE, n);

        int i;
        for(i=0; i<n ;i++)
        {
            argv[i] = va_arg(ar, VALUE);
        }
    } 

    NewArguments arg(class_name, 0, 0);

    arg.n    = n;
    arg.argv = argv;

    int error = 0;
    VALUE self = rb_protect(create_object_protect, reinterpret_cast<VALUE>(&arg), &error);

    if(error)
    {
        std::stringstream strm;
        strm << "Error creating Ruby class '" << class_name << "'";

        Exception e(strm.str().c_str());
        e.backtrace();
        throw e;
    }

    return self;
}

void require_class(mrb_state* mrb, VALUE x, VALUE cls)
{
    if(rb_obj_is_instance_of(x,cls) == Qfalse)
    {
        rb_raise( rb_eRuntimeError, 
                  "wrong argument type %s (expected %s)",
                  rb_obj_classname(x),
                  rb_class2name(cls)
            );
    }
}

} // end namespace mruby
