#pragma once

#include "../Net/Logger.hpp"
#include "../Net/Socket.hpp"
#include <cstdint>
#include "../Net/InetAddr.hpp"
#include <sys/socket.h>
#include <functional>
#include <utility>
#include <atomic>

namespace NS_UDP_SERVER_MODULE
{
    using namespace NS_LOGGER_MUDULE;
    using namespace NS_SOCKET_MODULE;
    using handler_t = std::function<std::vector<char>(std::vector<char>&, const InetAddr& peer)>;
    class UdpServer
    {
    public:
        UdpServer(uint16_t port)
            :_udpfd(new UdpSocket)
            ,_running(false)
        {
            bool success =_udpfd->BuildUdpServer(port);
            if(!success)
            {
                LOG(LogLevel::FATAL) << "基础服务器构建失败";
            }
            LOG(LogLevel::DEBUG) << "基础服务器构建成功";
        }

        ~UdpServer()
        {
            delete _udpfd;
        }

        bool Register(const handler_t& handler)
        {
            _handler = handler;
            if(!_handler)
            {
                LOG(LogLevel::WARNING) << "注册基础服务器处理方法失败";
                return false;
            }
            LOG(LogLevel::DEBUG) << "注册基础服务器处理方法成功";
            return true;
        }

        void Loop()
        {
            _running = true;
            while(_running)
            {
                std::vector<char> recv_buf;
                InetAddr peer;
                int n = _udpfd->Recv(&recv_buf, &peer);

                if(n > 0)
                {
                    Service(std::move(recv_buf), peer);
                }
            }
        }

        void Stop()
        {
            _running = false;
        }

        Socket* Get()
        {
            return _udpfd;
        }

    private:
        bool Service(std::vector<char> recv_buf, const InetAddr& addr)
        {
            // udp 传输层缓冲区为数据报，没有数据拆分，不用循环读
            std::vector<char> send_buf;
            send_buf = _handler(recv_buf, addr);
            if(!send_buf.empty())
            {
                int n = _udpfd->Send(send_buf, addr);
                if(n > 0)
                {
                    return true;
                }
            }
            return false;
        }
    private:
        handler_t _handler;
        Socket* _udpfd;
        std::atomic_bool _running;
    };
}
