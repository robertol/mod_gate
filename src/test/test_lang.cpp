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

/*
TEST_F(UnitTest, CompileText)
{
    ruby::VM vm;

    if(vm.executeCode("puts 'executeCode()'") == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }
}
*/

TEST_F(UnitTest, SQLiteApi)
{
    ruby::VM vm;

    // Global Ruby variables to pass in test dir paths
    vm.setGlobalVariable("$modgate_test_dir", MODGATE_TEST_DIR);

    // Load source SQLite Ruby module
    const char* sqlite_module = MODGATE_RUBY_LIB_DIR"/sqlite.rb";
    if(vm.executeFile(sqlite_module) == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }

    /*
    if(vm.loadExtensions() == false)
    {
        cerr << vm.error();
        cerr << vm.backtrace();
    }
    */

    if(vm.executeFile("scripts/test_lang/io/1.rb") == false)
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
