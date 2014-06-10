#ifndef MRUBY_CPP_LIBRARY_H
#define MRUBY_CPP_LIBRARY_H

#if defined(__clang__)
#define RUBY_DONT_SUBST 1
#endif

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/variable.h>
#include <mruby/value.h>
#include <mruby/proc.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/debug.h>
#include <mruby/error.h>
#include <mruby/data.h>

#include <stdexcept>
#include <string>
#include <map>

#define VALUE mrb_value

namespace mruby {

//------------------------------------------------------------------------------
// Object Wrapper
//------------------------------------------------------------------------------

/** This class implements a C handle for a Ruby Object. That is, the object
 ** instantiated is defined within a Ruby script or library; it exists within the
 ** Ruby language. This class creates and holds an instance of the class given by
 ** the name argument in the constructor. 
 **

 ** It holds this instance in memory, keeping it safe from the Ruby garbage
 ** collector (GC), by registering it in the underlying ruby::register_object
 ** API. This keeps the object in a Ruby array, which in turn keeps an active
 ** reference to that object, which keeps the GC from reaping it.
 */

class Object
{
  private:

    mrb_state* mrb;

    Object();

  protected:

    VALUE self;
    std::string _class_name;

  public:

    Object(mrb_state* vm, const char* name, int n=0, ...);
    virtual ~Object();

    VALUE method(const char* name, int n=0, ...);

    const char* class_name();

    inline VALUE value() { return self; }
};

//------------------------------------------------------------------------------
// Memory Management
//------------------------------------------------------------------------------

// Ruby's garbage collector must know about our non-exported ruby instances in
// C++. Otherwise we can suddenly get killed without warning. In the "Pragmatic
// Programmers Guide" section "Sharing Data Between Ruby and C" they recommend C
// objects being registered in a global array.

class Objects
{
  private:

    static VALUE objects;

    mrb_state* mrb;

    Objects(mrb_state* mrb);
    ~Objects();

    static Objects* singleton;

  public:

    static Objects* instance();
    static void register_object(mrb_state* mrb, VALUE object);
    static void free_object(mrb_state* mrb, VALUE object);
    static void free_all(mrb_state* mrb);
};

void register_object(mrb_state* mrb, VALUE object);
void free_object(mrb_state* mrb, VALUE object);
void free_all(mrb_state* mrb);

//------------------------------------------------------------------------------
// Exceptions
//------------------------------------------------------------------------------

/* Translate a ruby exception into an c++ exception. 
** 
** Example:
**
** void Test()
** {
**     int error = 0;
**       rb_protect(WrapTest, reinterpret_cast<VALUE>(this), &error);
**
**       if(error)
**     {
**           throw ruby::Exception("error loading test.rb");
**     }
** }
*/

class Exception : public std::exception
{
    std::string _msg;
    std::string _backtrace;
    std::string _type;

  public:

    Exception(const char* msg=NULL);
    Exception(const Exception& e);

    virtual ~Exception() throw();

    const Exception& operator=(const Exception& e);

    void backtrace() throw();

    const char* type() const;
    virtual const char* what() const throw();
    const char* stackdump() const;
};

// Wrap rb_funcall
struct Arguments
{
    VALUE recv;
    mrb_sym id;
    int n;
    VALUE *argv;
};

/* Call a ruby function in a safe way. Translate ruby errors into c++
** exceptions. Instead of calling rb_funcall(), do this:
**
**    VALUE Safe() {
**        return ruby::method(
**            self, 
**            rb_intern("test"), 
**            1, 
**            INT2NUM(42)
**        );
**    }
*/
VALUE method_wrap(mrb_state* mrb, VALUE arg);
VALUE method(mrb_state* mrb, VALUE recv, mrb_sym id, int n, ...);
VALUE vm_method(mrb_state* mrb, VALUE recv, mrb_sym id, int n, va_list ar);

// Eval ruby string in a safe way
void eval( mrb_state* mrb, 
           const char* code, 
           const char* filename=NULL, 
           int sl=0, 
           VALUE binding=mrb_nil_value() );

// Require a ruby-file in a safe way
void require(mrb_state* mrb, const char* filename);

bool call_function(mrb_state* mrb, const char* method, int n, ...);

// Copies the contents of a Hash into an STL Map
bool copy_hash(mrb_state* mrb, VALUE hash, std::map<std::string, std::string>& map);

// Load a ruby-file in a safe way. If anonymous == 1, the loaded script will be
// executed under an anonymous module, protecting the calling program's global
// namespace.
void load(mrb_state* mrb, const char* filename, int anonymous=0);

// Ensure that x is of class cls
void require_class(mrb_state* mrb, VALUE x, VALUE cls);

// Create a new instance in a safe way
VALUE create_object(mrb_state* mrb, const char* class_name, int n, va_list ar);

} // end namespace mruby

#endif
