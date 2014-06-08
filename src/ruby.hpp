#ifndef MODJERK_RUBY_DECLARE
#define MODJERK_RUBY_DECLARE

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

#include <string>

namespace ruby
{

class VM
{
    // Disallow assignment operator and default copy ctor
    const VM &operator=(const VM &old);
    VM(const VM &old);

    mrb_state* my_vm;
    std::string my_error;
    std::string my_backtrace;

public:

    VM();
    virtual ~VM();

    // Loads optional compiled-in libraries (SQLite, etc.).
    bool loadExtensions();

    bool executeByteCode(const uint8_t* code);
    bool executeCode(const char* code);
    bool executeFile(const char* code);

    inline mrb_state* handle() { return my_vm; }
    inline const std::string& error() const { return my_error; }
    inline const std::string& backtrace() const { return my_backtrace; }

    void setGlobalVariable(const char* name, const char* value);

  private:

    void getBacktrace();
};

} // end namespace ruby

#endif
