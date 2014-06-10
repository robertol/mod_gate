#include <fstream>

#include "getopt.h"

#include "../../../../../../../include/platform.h"
#include "ruby.hpp"
#include "rhtml_parser.hpp"

using std::string;
using std::endl;
using std::stringstream;

//------------------------------------------------------------------------------
// Command line options
//------------------------------------------------------------------------------

static struct option opt_array[] = {
    { "version",   0,  NULL, 'v', "Show version"                                 },
    { "help",      0,  NULL, 'h', "Show help"                                    },
    {  NULL,       0,     0,  0,   NULL                                          }
};

void print_help()
{
    printf( "rhtml version %s (%s)\n\n"
            "Usage: rhtml [options]\n\n"
            "Available options:\n\n",
            MODGATE_RELEASE_VERSION, SYSTEM_ARCH );

    const option* i = opt_array;

    while(1)
    {
        if(i->name == NULL)
        {
            break;
        }

        if(i->has_arg != 0)
        {
            std::stringstream long_opt;
            long_opt << "--" << i->name << "=<" << i->name << ">";
            printf( "   -%c, %-20s %s\n", i->val, 
                    long_opt.str().c_str(), i->description);
        }
        else
        {
            printf("   -%c, --%-18s %s\n", i->val, i->name, i->description);
        }

        i++;
    }

    printf("\n");
}

typedef VALUE (*fn)(...);

extern "C"
{

int process_cmdline(int argc, char* const argv[])
{
    char ch;
    int long_opt_index = 0;
    bool debug_mode = false;

    while((ch = getopt_long(argc, argv, "dvh", opt_array, &long_opt_index)) != -1)
    {
        switch(ch)
        {
            case 'd':
            {
                debug_mode = true;                
            }

            case 'v':
            {
                printf("rhtml %s (%s)\n", MODGATE_RELEASE_VERSION, SYSTEM_ARCH);

                return 0;
            }

            case 'h':
            {
                print_help();

                return 0;
            }
        }
    }

    //mrb_set_argv(argc, (char**)argv);

    mrb_state* mrb = mrb_open();

    rsp::Rhtml2Parser parser(mrb);
    parser.raise_on_error = false;
    parser.eval_file(argv[1]);
    
    mrb_close(mrb);

    return 0;
}

}

//------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    mruby::startup("rhtml");

    if(process_cmdline(argc, argv) == 0)
    {
        return 1;
    }

    mruby::shutdown();

    return 0;
}
