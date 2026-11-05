#pragma once
#include <chrono>
#include <memory>
#include "../Net/Socket.hpp"
namespace NS_CONNECTIOM_MODULE
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

        std::shared_ptr<Socket> _sockfd;
        InetAddr _addr;
        std::string _id;
    };

}
