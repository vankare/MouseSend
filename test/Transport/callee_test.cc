#include "../../Transport/CalleeStrategy.hpp"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
using namespace NS_STRATEGY_MODULE;

std::function<std::vector<char>(const std::vector<char>&)> handler_data = 
[](const std::vector<char>& data) ->std::vector<char>
{
    std::ofstream ofs("test_callee.txt");
    if(!ofs.is_open())
    {
        std::cerr << "ifstream err" << std::endl;
    }

    ofs.write(data.data(), data.size());

    return std::vector<char>();
};

/*
 * 本测试中, callee 监听3000端口, signal_server 监听 8888
 */
TEST(Strategy, CalleeStrategy2)
{
    CalleeStrategy callee(
        "127.0.0.1",
        8888,
        handler_data,
        3000
    );

    callee.ConnectSignal("callee");
    callee.StartRecv();
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
