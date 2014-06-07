#include "ruby.hpp"

#include <stdlib.h>
#include <stdarg.h>

#include <string>
#include <iostream>

#include "ruby_sqlite.h"
#include "ruby_sqlite_stmt.h"

using std::string;
using std::cout;
using std::endl;

#define BUFF_LEN 1024

static void format(mrb_state *mrb, string& buffer, int level, const char *fmt, ...)
{
    char buff[BUFF_LEN];
	va_list ap;
	ssize_t vlen;
	ssize_t maxlen = BUFF_LEN - 1;

	va_start(ap, fmt);
	vlen = vsnprintf(&buff[0], maxlen, fmt, ap);
	va_end(ap);

    ssize_t bytes = 0;

	if(vlen >= maxlen)
    {
		buff[maxlen-1] = '\0'; /* Ensuring libc correctness */
        bytes = maxlen;
	}
    else if(vlen >= 0)
    {
		buff[vlen] = '\0'; /* Ensuring libc correctness */
        bytes = vlen;
	} 
    else
    {
        // The libc on this system is broken.
		vlen = sizeof("<broken vsnprintf>") - 1;
		maxlen--;
		bytes = vlen < maxlen ? vlen : maxlen;
		memcpy(buff, "<broken vsnprintf>", bytes);
		buff[bytes] = 0;
	}

    buffer.append(&buff[0], bytes);
}

static void output_backtrace(mrb_state *mrb, mrb_int ciidx, mrb_code *pc0, string& backtrace)
{
    mrb_callinfo *ci;
    const char *filename, *method, *sep;
    int i, lineno, tracehead = 1;
    
    if (ciidx >= mrb->c->ciend - mrb->c->cibase)
    {
        ciidx = 10; /* ciidx is broken... */
    }

    for (i = ciidx; i >= 0; i--)
    {
        ci = &mrb->c->cibase[i];
        filename = NULL;
        lineno = -1;
        
        if (!ci->proc)
        {
            continue;
        }
        
        if (MRB_PROC_CFUNC_P(ci->proc))
        {
            continue;
        }
        else
        {
            mrb_irep *irep = ci->proc->body.irep;
            mrb_code *pc;

            if (mrb->c->cibase[i].err)
            {
                pc = mrb->c->cibase[i].err;
            }
            else if (i+1 <= ciidx)
            {
                pc = mrb->c->cibase[i+1].pc - 1;
            }
            else
            {
                pc = pc0;
            }
            filename = mrb_debug_get_filename(irep, (uint32_t)(pc - irep->iseq));
            lineno = mrb_debug_get_line(irep, (uint32_t)(pc - irep->iseq));
        }

        if (lineno == -1) continue;
        
        if (ci->target_class == ci->proc->target_class)
        {
            sep = ".";
        }
        else
        {
            sep = "#";
        }
        
        if (!filename)
        {
            filename = "(unknown)";
        }
        
        if (tracehead)
        {
            //func(mrb, stream, 1, "trace:\n");
            tracehead = 0;
        }

        method = mrb_sym2name(mrb, ci->mid);
 
       if(method)
        {
            const char *cn = mrb_class_name(mrb, ci->proc->target_class);
            
            if(cn)
            {
                format(mrb, backtrace, 1, "\t[%d] ", i);
                format(mrb, backtrace, 0, "%s:%d:in %s%s%s", filename, lineno, cn, sep, method);
                format(mrb, backtrace, 1, "\n");
            }
            else
            {
                format(mrb, backtrace, 1, "\t[%d] ", i);
                format(mrb, backtrace, 0, "%s:%d:in %s", filename, lineno, method);
                format(mrb, backtrace, 1, "\n");
            }
        }
        else
        {
            format(mrb, backtrace, 1, "\t[%d] ", i);
            format(mrb, backtrace, 0, "%s:%d", filename, lineno);
            format(mrb, backtrace, 1, "\n");
        }
    }
}

namespace ruby
{

VM::VM() 
  : my_vm(NULL), 
    my_error(), 
    my_backtrace()
{
    my_vm = mrb_open();
     
    RClass* mod = mrb_define_module(my_vm, "SQLite");
    init_sqlite3(my_vm, mod);
    init_sqlite3_stmt(my_vm, mod);
}

VM::~VM()
{
    if(my_vm != NULL)
    {
        mrb_close(my_vm);
        my_vm = NULL;
    }
}

bool VM::executeCode(const char* code)
{
    mrb_sym zero_sym = mrb_intern_lit(my_vm, "$0");
    mrbc_context *c = mrbc_context_new(my_vm);
    mrbc_filename(my_vm, c, "inproc-code");

    mrb_value v = mrb_load_string_cxt(my_vm, code, c);
    mrbc_context_free(my_vm, c);

    if(my_vm->exc)
    {
        getBacktrace();

        return false;
    }

    return true;
}

bool VM::executeFile(const char* filename)
{
    mrbc_context *c = mrbc_context_new(my_vm);
    mrb_sym zero_sym = mrb_intern_lit(my_vm, "$0");
    mrbc_filename(my_vm, c, filename);

    FILE* file = fopen(filename, "r");
    mrb_value v = mrb_load_file_cxt(my_vm, file, c);
    fclose(file);

    mrbc_context_free(my_vm, c);
    if(my_vm->exc)
    {
        getBacktrace();

        return false;
    }

    return true;
}

void VM::getBacktrace()
{
    if(my_vm->exc)
    {
        mrb_int ciidx = mrb_fixnum(
            mrb_obj_iv_get(my_vm, my_vm->exc, mrb_intern_lit(my_vm, "ciidx")));                
        
        mrb_code* code = (mrb_code*)mrb_cptr(
            mrb_obj_iv_get( my_vm, 
                            my_vm->exc, 
                            mrb_intern_lit(my_vm, "lastpc")));
        
        my_backtrace.clear();
        my_error.clear();
        
        output_backtrace(my_vm, ciidx, code, my_backtrace);           
        mrb_value s = mrb_funcall(my_vm, mrb_obj_value(my_vm->exc), "inspect", 0);
        if(mrb_string_p(s))
        {
            my_error.append(RSTRING_PTR(s), RSTRING_LEN(s));
        }
    }
}

} // end namespace ruby
