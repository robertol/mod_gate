#include <sstream>
#include <fstream>

#include "platform.h"
#include "getopt.h"
#include "sqlite.hpp"

using namespace sqlite;
using std::string;
using std::endl;
using std::stringstream;

//------------------------------------------------------------------------------
// Command line options
//------------------------------------------------------------------------------

static struct option opt_array[] = {
    { "check",     0,  NULL, 'c', "Check database"        },
    { "index",     0,  NULL, 'i', "Index database"        },
    { "stats",     0,  NULL, 's', "Print statistics"      },
    { "version",   0,  NULL, 'v', "Show version"          },
    { "help",      0,  NULL, 'h', "Show help"             },
    {  NULL,       0,     0,  0,   NULL                   }
};

void print_help()
{
    printf( "jerkdb version %s (%s)\n\n"
            "Usage: jerkdb [options]\n\n"
            "Available options:\n\n",
            MODJERK_RELEASE_VERSION, SYSTEM_ARCH );

    const option* i = opt_array;

    while(1)
    {
        if(i->name == NULL)
        {
            break;
        }

        if(i->has_arg != 0)
        {
            stringstream long_opt;
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

int process_cmdline(int argc, char* const argv[])
{
    char ch;
    int long_opt_index = 0;
    bool arg_provided  = false;

    while((ch = getopt_long(argc, argv, "cisvxh", opt_array, &long_opt_index)) != -1)
    {
        switch(ch)
        {
            case 'c':
            {
                printf("Check\n");
            }
            
            case 'i':
            {
                printf("Index\n");
            }

            case 's':
            {
                printf("Stats\n");
            }

            case 'v':
            {
                printf("modjerk %s (%s)\n", MODJERK_RELEASE_VERSION, SYSTEM_ARCH);

                return 0;
            }

            case 'h':
            {
                print_help();

                return 0;
            }
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    return process_cmdline(argc, argv);
}
