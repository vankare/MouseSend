#pragma once
#include "../Net/Server.hpp"
#include <vector>
#include <cstdint>
#include "../Message/Message.hpp"


namespace NS_STRATEGY_MODULE 
{
    using namespace NS_MESSAGE_MODULE;
    using namespace NS_SERVER_MODULE;
    const uint64_t g_init_recv_buf_size = 40960;   // 预开辟接收缓冲区大小
    class ServerStrategy
    {
    public:
        /*
         * handler_t 处理套接字读到的数据
         * handler_msg_t 处理协议解析后的数据
         * handler_data_t 处理业务数据
         */

        ServerStrategy(uint16_t port)
            :_svr(port)
        {  
            _svr.Register(
                [this](std::vector<char> req, const InetAddr& peer) -> std::vector<char>
                {
                    return this->_protocol.ParseProtocol(req, peer);
                }
            );

            _protocol.Register(
                [this](const Message& req, const InetAddr& peer) -> Message
                {
                    return Answer(req, peer);
                }
            );

            _data.reserve(g_init_recv_buf_size);

            LOG(LogLevel::DEBUG) << "注册处理数据方法成功";
        }

        void StartRecv()
        {
            _svr.Loop();
        }

        void Stop()
        {
            _svr.Stop();
        }

        Socket* GetSockfd()
        {
            return _svr.Get();
        }

        virtual Message Answer(Message in_msg, const InetAddr& peer_ddr) = 0;
    protected:
        Server _svr;                         // 基础服务器
        Protocol _protocol;                  // 协议
        std::vector<char> _data;             // 接收到的数据
    };
}
