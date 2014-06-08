#include <iostream>

#include <gtest/gtest.h>

#include "config.h"

#include <mruby/dump.h>
#include <mruby/irep.h>

#include "../ruby.hpp"
#include "../util.hpp"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include "sqlite_lib.c"

class UnitTest : public testing::Test
{
  public:

    UnitTest() : Test()
    {
        
    }

    virtual ~UnitTest()
    {
        
    }

  protected:  

    int my_data;

    void testMethod()
    {
        ASSERT_EQ(my_data, 101);
    }
};

TEST_F(UnitTest, LowLevelCompile)
{
    ruby::VM vm;

    // Global Ruby variables to pass in test dir paths
    vm.setGlobalVariable("$modjerk_test_dir", MODJERK_TEST_DIR);

    /*
    // Load bytes-compiled SQLite Ruby module
    if(vm.loadExtensions() == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }
    */

    // Load source SQLite Ruby module
    const char* sqlite_module = MODJERK_RUBY_LIB_DIR"/sqlite.rb";
    if(vm.executeFile(sqlite_module) == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }

    if(vm.executeFile("scripts/test_lang/io/1.rb") == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }

    if(vm.executeCode("puts 'executeCode()'") == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
