#include <iostream>

#include <gtest/gtest.h>

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

TEST_F(UnitTest, LowLevelCompile)
{
    ruby::VM vm;

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
