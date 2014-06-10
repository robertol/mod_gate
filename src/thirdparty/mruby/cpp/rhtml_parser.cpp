#include <string.h>

#include "ruby.hpp"
#include "rhtml_parser.hpp"

using namespace std;
using namespace rsp;

template<class T>
int inline replace(T& source, const char* find, const char* replace)
{
    int num=0;
    size_t fLen = strlen(find);
    size_t rLen = strlen(replace);
    for(typename T::size_type pos=0; (pos=source.find(find, pos))!=T::npos; pos+=rLen)
    {
        num++;
        source.replace(pos, fLen, replace);
    }

    return num;
}

// _context is a generic Ruby Object using the C++ ruby::Object class. We then
// get its binding. We use the binding as the global context for the RHTML
// document. We do this by passing it in for every call to eval() below.

// Note: do NOT initialize scanner(NULL) here. It will cause a segfault. This
// has something to do with flex code.

Rhtml2Parser::Rhtml2Parser(mrb_state* vm) 
  : raise_on_error(true),
    error(), text(), line(), 
    mrb(vm),
    _context("Object"), _binding(mrb_nil_value()), 
    _state(TEXT_STATE), _block_start_line(1), _current_line(1),
    _file_name(NULL)
{    
    
}

void Rhtml2Parser::append(const char* t)
{
    line += t;
}

void Rhtml2Parser::read(const char* t)
{
    switch (_state)
    {
        case TEXT_STATE:
        {
            // If we are not at the start of a new line, we are starting a new
            // ruby expression so we must use a semicolon.
            if(line.size() == 0)
            {
                line += "print \"";
            }

            string tmp = t;
            replace(tmp, "\\", "\\\\");
            replace(tmp, "\"", "\\\"");
            replace(tmp, "#", "\\#");
            line += tmp;

            break;
        }       

        default:
        {
            line += t;
        }
    }
}

const char* Rhtml2Parser::content_name()
{
    switch (_state)
    {
        case CODE_STATE:
        {
            return "code";
        }

        case COMMENT_STATE:
        {
            return "comment";
        }

        case TEXT_STATE:
        {
            return "text";
        }

        case VAR_STATE:
        {
            return "var";
        }
    }

    return NULL;
}

void Rhtml2Parser::change(state_t s)
{
    switch (_state)
    {
        case VAR_STATE:
        case CODE_STATE:
        {
            if(line.size() > 0)
            {
                line += "; ";
            }

            line += "print \"";
            
            break;
        }

        case COMMENT_STATE:
        {
            break;
        }

        case TEXT_STATE:
        {
            // If we are going from text state and a text line is already
            // started, then we need to close it.
            if(line.size() > 0)
            {
                line += "\"; ";
            }

            break;
        }
    }

    _state = s;
    _block_start_line = _current_line;
}

void Rhtml2Parser::end_line()
{
    _current_line++;

    switch(_state)
    {
        case CODE_STATE:
        {
            line += "\n";

            break;
        }

        case COMMENT_STATE:
        {
            break;
        }

        case TEXT_STATE:
        {
            // In case there was no text on this line to trigger a read(), then
            // we have to put in a print here for text blocks.
            if(line.size() == 0)
            {
                line += "print \"";
            }

            line += "\\n\"\n";

            break;
        }

        case VAR_STATE:
        {
            break;
        }
    }

    text += line;
    line.clear();
}

int Rhtml2Parser::current_line()
{
    return _current_line;
}

int Rhtml2Parser::block_start_line()
{
    return _block_start_line;
}


bool Rhtml2Parser::eval(VALUE b)
{
    //> Setup Ruby binding (global scope for eval())
    VALUE binding = mrb_nil_value();

    if(mrb_nil_p(b))
    {
        // We weren't passed a binding, so we need to create one ourselves.
        _binding = _context.method("binding");

        // Put a reference on it so it doesn't get GC'd.
        mruby::register_object(_binding);
    }

    try
    {
        mruby::eval(text.c_str(), _file_name, _block_start_line, _binding);
    }
    catch(const mruby::Exception &e)
    {
        fprintf(stderr, "\nERROR : %s (lines %i-%i)\n%s\n%s\n",
                _file_name,
                _block_start_line,
                _current_line,
                e.what(),
                e.stackdump() );
        
        std::stringstream strm;
        strm << "Error: " << _file_name << "(lines " << _block_start_line 
             << "-" << _current_line << ")\n" << e.what();

        error = strm.str();

        if(raise_on_error == true)
        {
            mrb_exc_raise( mrb, 
                           mrb_gv_get(mrb, mrb_intern_cstr(mrb, "$!")) );
        }
        
        return false;
    }
        
    //mruby::method( rb_stdout, rb_intern("write"), 1, 
    //              rb_str_new(text.c_str(), text.size()) );

    // Flush Ruby standard output.
    //mruby::method(rb_stdout, rb_intern("flush"), 0);

    if(mrb_nil_p(b))
    {
        // Release binding
        mruby::free_object(binding);
    }

    return true;
}

bool Rhtml2Parser::eval_file(const char* filename, VALUE binding)
{
    compile_file(filename);

    return eval(binding);
}

bool Rhtml2Parser::eval_text(const char* str, VALUE binding)
{
    compile_text(str);

    return eval(binding);
}
