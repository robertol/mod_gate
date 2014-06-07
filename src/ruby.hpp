#ifndef MODJERK_APR_POOL_DECLARE
#define MODJERK_APR_POOL_DECLARE

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

    bool executeCode(const char* code);
    bool executeFile(const char* code);

    inline const std::string& error() const { return my_error; }
    inline const std::string& backtrace() const { return my_backtrace; }

  private:

    void getBacktrace();
};

} // end namespace ruby

#endif
