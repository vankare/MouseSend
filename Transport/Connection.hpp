#pragma once
#include <chrono>
#include <sys/socket.h>
#include "../Net/Socket.hpp"
namespace NS_CONNECTION_MODULE
{
    using namespace NS_SOCKET_MODULE;
    struct Connection
    {
        Connection() = default;

        Connection(const std::string& id, const InetAddr& addr)
            :_id(id)
            ,_addr(addr)
            ,_sockfd(new UdpSocket)
        {  
            _sockfd->BuildUdpClient();
        }

        // 传入一个已有的套接字，不需要再创建套接字
        Connection(const std::string& id, Socket* const sockfd, const std::string& ip, uint16_t port)
            :_id(id)
            ,_sockfd(sockfd)
            ,_addr(ip, port)
        {  }

        int Send(std::vector<char> send_buf)
        {
            return _sockfd->Send(send_buf, _addr);
        }

        int Recv(std::vector<char>* recv_buf)
        {
            return _sockfd->Recv(recv_buf, nullptr);
        }

        bool RecvTimeOut(const std::chrono::milliseconds& ms)
        {
            return _sockfd->SetRecvTimeout(ms);
        }

        std::string AddrStr() const
        {
            return _addr.ToString();
        }

        Socket* _sockfd;
        InetAddr _addr;
        std::string _id;
    };
}
