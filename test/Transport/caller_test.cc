#include "../../Transport/CallerStrategy.hpp"
#include <gtest/gtest.h>

using namespace NS_STRATEGY_MODULE;

//TEST(TestStrategy, TestClientStrategy)
//{
//    Connection conn("123", {"127.0.0.1", 3000});
//    std::string test_str("123456789");
//    std::vector<char> send_data(test_str.begin(), test_str.end());
//
//    CallerStrategy caller(conn);
//    caller.StartSend(send_data);
//}

TEST(TestStrategy, CallerStrategy)
{
    CallerStrategy caller("127.0.0.1", 8888);
    caller.ConnectSignal();
    caller.GetCalleeAddr("caller", "callee");

    std::string test_str = "1234567890";
    caller.StartSend(std::vector<char>(test_str.begin(), test_str.end()));
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
