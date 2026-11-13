#include "../../Config/Config.hpp"
#include <gtest/gtest.h>
using namespace NS_CONFIG_MODULE;

TEST(Config, Server)
{
    EXPECT_EQ(ServerConfig::_port, 4001);
    EXPECT_EQ(ServerConfig::_connection_exired_timeout, minute_t(30));
}

TEST(Config, Client)
{
    EXPECT_EQ(CalleeConfig::_port, 0);
    EXPECT_EQ(CalleeConfig::_id, "");

    EXPECT_EQ(CallerConfig::_send_chunk_size, 8192);
    EXPECT_EQ(CallerConfig::_send_win_size, 4);
    EXPECT_EQ(CallerConfig::_max_fails, 3);
    EXPECT_EQ(CallerConfig::_send_recv_timeout, millisecond_t(500));
}

int main()
{
    LOAD_CLIENT_CONFIG("../../Config/client.toml");
    LOAD_SERVER_CONFIG("../../Config/server.toml");

    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
