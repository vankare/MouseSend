#include "../Server.hpp"
#include <gtest/gtest.h>
using namespace NS_SERVER_MODULE;

TEST(TestServer, Loop)
{
    Server svr(0);
    svr.Register(
        [](const std::vector<char>& in) -> std::vector<char>
        {
            return in;
    });
    svr.Loop();
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
