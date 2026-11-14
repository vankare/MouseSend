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
    EXPECT_FALSE(ClientConfig::_id.empty());
    EXPECT_EQ(ClientConfig::_server_ip, "127.0.0.1");
    EXPECT_EQ(ClientConfig::_server_port, 4001);
    EXPECT_EQ(ClientConfig::_max_fails, 3);
    EXPECT_EQ(ClientConfig::_send_recv_timeout, millisecond_t(500));

    EXPECT_EQ(CalleeConfig::_port, 0);

    EXPECT_EQ(CallerConfig::_send_chunk_size, 8 * 1024);
    EXPECT_EQ(CallerConfig::_send_win_size, 4);
    EXPECT_EQ(CallerConfig::_read_file_size, 10 * 1024 * 1024);
}

int main()
{
    LOAD_CLIENT_CONFIG("../../Config/client.toml");
    LOAD_SERVER_CONFIG("../../Config/server.toml");

    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
