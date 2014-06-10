#ifndef RSP_RHTML_DECLARE
#define RSP_RHTML_DECLARE

#include <string>
#include <sstream>

#include "ruby.hpp"

namespace rsp
{

struct yy_buffer_state;

/** 
This is a rhtml that works like eRuby. It is a stripped down, minimalist
version which that doesn't know anything about CGI or HTTP per se, just parsing
and executing RHTML code.

There are significant differences between eRuby and ERB/erubis in that the
former processes puts/print statements "correctly" and the latter do not. That
is, a puts statement in RHTML code should stream its output in the order in
which is is located in the RHTML document. THIS DOES NOT WORK IN
ERB/erubis. Rather, these latter two will stream all output from puts at the
head of the document -- out of order. I consider this a design flaw. Only eRuby
does this correctly.

Therefore, this rhtml is modeled after eRuby.
*/

class Rhtml2Parser
{
    // Disallow assignment operator and default copy ctor
    const Rhtml2Parser &operator=(const Rhtml2Parser &old); 
    Rhtml2Parser(const Rhtml2Parser &old);

    // A handle to the scanner
    void* scanner;

  public:

    bool raise_on_error;

    enum state_t
    {
        CODE_STATE    = 1,
        VAR_STATE     = 2,
        COMMENT_STATE = 3,
        TEXT_STATE    = 4
    };

    std::string error;
    std::string text;
    std::string line;

  private:

    mrb_state* mrb;

    mruby::Object _context;
    VALUE _binding;
    state_t _state;
    int _block_start_line;
    int _current_line;
    const char* _file_name;

  public:

    Rhtml2Parser(mrb_state* vm);

    /** Process file contents */
    bool compile_file(const char* filename);
    bool eval_file(const char* filename, VALUE binding=mrb_nil_value());

    state_t state() { return _state; }

    /** Process chunk of text */
    bool compile_text(const char* str);
    bool eval_text(const char* str, VALUE binding=mrb_nil_value());

    void change(state_t s);
    void append(const char* text);
    void read(const char* text);
    void end_line();

  protected:

    bool eval(VALUE b);

    const char* content_name();
    void init();
    int finalize();

    int current_line();
    int block_start_line();
};

} // end namespace rsp

#endif
