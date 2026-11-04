#pragma once

#include "../Comm/Logger.hpp"
#include "../Comm/Socket.hpp"
#include <cstdint>
#include <memory>
#include "../Comm/InetAddr.hpp"
#include <sys/socket.h>
#include <functional>
#include <utility>

namespace NS_SERVER_MODULE
{
    using namespace NS_LOGGER_MUDULE;
    using namespace NS_SOCKET_MODULE;
    using handler_t = std::function<std::vector<char>(std::vector<char>&)>;
    class Server
    {
    public:
        Server(uint16_t port)
            :_udpfd(new UdpSocket)
        {
            bool success =_udpfd->BuildUdpServer(port);
            if(!success)
            {
                LOG(LogLevel::FATAL) << "服务器构建失败";
            }
            LOG(LogLevel::DEBUG) << "服务器构建成功";
        }

        bool Register(const handler_t& handler)
        {
            _handler = handler;
            if(!_handler)
            {
                LOG(LogLevel::WARNING) << "注册处理方法失败";
                return false;
            }
            LOG(LogLevel::DEBUG) << "注册处理方法成功";
            return true;
        }

        void Loop()
        {
            while(true)
            {
                std::vector<char> recv_buf;
                InetAddr peer;
                int n = _udpfd->Recv(&recv_buf, &peer);

                if(n > 0)
                {
                    Service(std::move(recv_buf), peer);
                }
                else
                {
                    LOG(LogLevel::ERRNO) << "接收数据失败";
                }
            }
        }
    private:
        bool Service(std::vector<char> recv_buf, const InetAddr& addr)
        {
            // udp 传输层缓冲区为数据报，没有数据拆分，不用循环读
            std::vector<char> send_buf;
            uint64_t status = 0;
            InetAddr target;
            send_buf = _handler(recv_buf);
            if(!send_buf.empty())
            {
                int n = _udpfd->Send(send_buf, target);
                if(n > 0)
                {
                    LOG(LogLevel::INFO) << "成功处理一条请求";
                    return true;
                }
            }
            return false;
        }
    private:
        handler_t _handler;
        std::unique_ptr<Socket> _udpfd;
    };
}
