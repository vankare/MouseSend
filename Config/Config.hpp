#pragma once
#include "toml.hpp"
#include "chrono"
#include "../Net/Util.hpp"
#include "../Net/Logger.hpp"

namespace NS_CONFIG_MODULE
{
    using namespace NS_UTIL_MODULE;
    using namespace NS_LOGGER_MUDULE;
    using millisecond_t = std::chrono::milliseconds;
    using minute_t = std::chrono::minutes;

    struct ServerConfig
    {
        ServerConfig(const std::string& server_toml = "server_toml")
        {
            if(!FileUtil::IsExists(server_toml))
            {
                LOG(LogLevel::WARNING) << "配置文件不存在, 使用默认值";
                return;
            }
            auto config = toml::parse_file(server_toml);
            _port = config["port"].value_or(_port);
            _connection_exired_timeout = minute_t(
                config["connection_expired_timeout"].value_or<int64_t>(
                _connection_exired_timeout.count())
            );
        }
        inline static uint16_t _port = 4001;
        inline static minute_t _connection_exired_timeout = minute_t(30);
    };

    struct ClientConfig
    {
        ClientConfig(const std::string& client_toml = "")
        {
            if(!FileUtil::IsExists(client_toml))
            {
                LOG(LogLevel::WARNING) << "配置文件不存在, 使用默认值";
                return;
            }
            auto config = toml::parse_file(client_toml);

            std::string unique_id = StringUtil::UniqueString();
            _id = config["id"].value_or(unique_id);
            if(_id.empty())
            {
                _id = unique_id;
            }
            if(_id == unique_id || _id.empty())
            {
                LOG(LogLevel::WARNING) << "没有设置 id, 生成 id" << unique_id;
            }
            else
            {
                LOG(LogLevel::INFO) << "id 为: " << _id;
            }

            _max_fails = config["max_fails"].value_or(_max_fails);
            _send_recv_timeout = millisecond_t(
                config["send_recv_timeout"].value_or<int64_t>(
                _send_recv_timeout.count())
            );
            _server_ip = config["server_ip"].value_or(_server_ip);
            _server_port = config["server_port"].value_or(_server_port);
        }
        inline static std::string _id = "";
        inline static std::string _server_ip = "127.0.0.1";
        inline static uint16_t _server_port = 4001;
        inline static uint16_t _max_fails = 3;
        inline static millisecond_t _send_recv_timeout = millisecond_t(500);
    };

    struct CallerConfig : ClientConfig
    {

        CallerConfig(const std::string& client_toml = "")
            :ClientConfig(client_toml)
        {
            if(!FileUtil::IsExists(client_toml))
            {
                LOG(LogLevel::WARNING) << "配置文件不存在, 使用默认值";
                return;
            }
            auto config = toml::parse_file(client_toml);
            _send_chunk_size = config["send_chunk_size"].value_or(_send_chunk_size) * 1024;
            _send_win_size = config["send_win_size"].value_or(_send_win_size);
            _read_file_size = config["read_file_size"].value_or(_read_file_size) * 1024 * 1024;
        }

        inline static uint32_t _send_chunk_size = 8;
        inline static uint16_t _send_win_size = 4;
        inline static uint32_t _read_file_size = 10;
    };

    struct CalleeConfig : ClientConfig
    {
        CalleeConfig(const std::string& client_toml = "")
            :ClientConfig(client_toml)
        {
            if(!FileUtil::IsExists(client_toml))
            {
                LOG(LogLevel::WARNING) << "配置文件不存在, 使用默认值";
                return;
            }
            auto config = toml::parse_file(client_toml);
            _port = config["port"].value_or(_port);
        }
        inline static uint16_t _port = 0;
    };
}

#define LOAD_SERVER_CONFIG(...) (NS_CONFIG_MODULE::ServerConfig(__VA_ARGS__))
#define LOAD_CLIENT_CONFIG(...) \
do       \
{        \
    NS_CONFIG_MODULE::CalleeConfig(__VA_ARGS__);   \
    NS_CONFIG_MODULE::CallerConfig(__VA_ARGS__);   \
}        \
while(0)
