#include "../TransportStrategy.hpp"
#include <gtest/gtest.h>

using namespace NS_PROTOCOL_MODULE;

TEST(TestStrategy, TestClientStrategy)
{
    Connection conn("123", {"127.0.0.1", 3000});
    std::string test_str("123456789");
    std::vector<char> send_data(test_str.begin(), test_str.end());

    CallerStrategy caller(conn);
    caller.StartSend(send_data);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
