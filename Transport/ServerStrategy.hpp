#pragma once
#include "../Net/Server.hpp"
#include <vector>
#include <functional>
#include <cstdint>
#include "../Message/Message.hpp"


namespace NS_STRATEGY_MODULE 
{
    using namespace NS_MESSAGE_MODULE;
    using namespace NS_SERVER_MODULE;
    using handler_data_t = std::function<std::vector<char>(std::vector<char>)>;
    const uint64_t g_init_recv_buf_size = 40960;   // 预开辟接收缓冲区大小
    class ServerStrategy
    {
    public:
        /*
         * handler_t 处理套接字读到的数据
         * handler_msg_t 处理协议解析后的数据
         * handler_data_t 处理业务数据
         */

        ServerStrategy(handler_data_t handler_data, uint16_t port = 0)
            :_svr(port)
            ,_handler_data(handler_data)
        {  
            _svr.Register(
                [this](std::vector<char> req) -> std::vector<char>
                {
                    return this->_protocol.ParseProtocol(req);
                }
            );

            _protocol.Register(
                [this](const Message& req) -> Message
                {
                    std::vector<char> resp_vec = this->_handler_data(req.GetPayload());
                    return Message(ACK, req.GetSeq(), resp_vec);
                }
            );

            _data.reserve(g_init_recv_buf_size);

            LOG(LogLevel::DEBUG) << "注册处理数据方法成功";
        }

        void StartRecv()
        {
            _svr.Loop();
        }

        virtual Message Answer(Message in_msg) = 0;
    protected:
        Server _svr;                         // 基础服务器
        Protocol _protocol;                  // 协议
        handler_data_t _handler_data;        // 处理数据方法(外部注册)
        std::vector<char> _data;             // 接收到的数据
    };
}
