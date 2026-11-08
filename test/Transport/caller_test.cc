#include "../../Transport/CallerStrategy.hpp"
#include "../../Net/Util.hpp"
#include <gtest/gtest.h>
using namespace NS_UTIL_MODULE;

using namespace NS_STRATEGY_MODULE;


TEST(TestStrategy, CallerStrategy)
{
    CallerStrategy caller("caller", "127.0.0.1", 8888);

    caller.ConnectSignal();
    caller.GetCalleeAddr("callee");

   auto test_data = FileUtil::ReadFile("test_in.bin");

    EXPECT_TRUE(caller.StartSend(test_data));
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
