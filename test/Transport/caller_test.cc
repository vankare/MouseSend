#include "../../Transport/CallerStrategy.hpp"
#include "../../Net/Util.hpp"
#include <gtest/gtest.h>
using namespace NS_UTIL_MODULE;
using namespace NS_STRATEGY_MODULE;


TEST(TestStrategy, CallerStrategy)
{
   auto test_data = FileUtil::ReadFile("test_in.bin");

    CallerStrategy caller("caller", "39.107.191.147", 4000);

    caller.ConnectSignal();
    caller.WaitForPeer("callee");
    caller.HolePunching();

    EXPECT_TRUE(caller.StartSend(test_data));
}

//TEST(TestStrategy, LocalTest)
//{
//   auto test_data = FileUtil::ReadFile("test_in.bin");
//
//    CallerStrategy caller("caller", "127.0.0.1", 4000);
//
//    caller.ConnectSignal();
//    caller.WaitForPeer("callee");
//    caller.HolePunching();
//
//    EXPECT_TRUE(caller.StartSend(test_data));
//}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
