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
 * 本测试中, callee 监听3000端口, signal_server 监听 8888
 */
TEST(Strategy, CalleeStrategy2)
{
    CalleeStrategy callee(
        "callee",
        "127.0.0.1",
        8888,
        handler_data
    );

    callee.ConnectSignal();
    callee.StartRecv();
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
