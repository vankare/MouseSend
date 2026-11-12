#include "../../Transport/CalleeStrategy.hpp"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include "../../Net/Util.hpp"
using namespace NS_UTIL_MODULE;


using namespace NS_STRATEGY_MODULE;

std::function<void(const std::vector<char>&)> handler_data = 
[](const std::vector<char>& data)
{
    FileUtil::WriteFile("test_out.bin", data);
};

/*
 * 本测试中, callee 监听随机端口, signal_server 监听 4000 端口
 */
TEST(Strategy, CalleeStrategy2)
{
    CalleeStrategy callee(
        "callee",
        "39.107.191.147",
        4000,
        handler_data
    );

    callee.ConnectSignal();
    callee.WaitForPeer("caller");
    callee.HolePunching();
    callee.StartRecv();
}

//TEST(Strategy, LocalTest)
//{
//    CalleeStrategy callee(
//        "callee",
//        "127.0.0.1",
//        4000,
//        handler_data
//    );
//
//    callee.ConnectSignal();
//    callee.WaitForPeer("caller");
//    callee.HolePunching();
//    callee.StartRecv();
//}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
