#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <mutex>
#include "../Comm/Socket.hpp"

namespace NS_CONNECTIOM_MODULE
{
    // 最大失败次数
    const int g_max_fails = 3;
    // 超时, 默认5000毫秒
    const std::chrono::milliseconds g_timeout(5000);

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

        std::shared_ptr<Socket> _sockfd;
        InetAddr _addr;
        std::string _id;
    };

    class ConnectionManager
    {
    public:
        void AddConnection(const std::string& id, const InetAddr& addr)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) != _connections.end())
            {
                _connections.emplace(id, Connection(id, addr));
            }
        }

        void DelConnection(const std::string id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) != _connections.end())
            {
                _connections.erase(id);
            }
        }

        bool GetConnById(const std::string& id, Connection* out)
        {
            if(out == nullptr)
            {
                return false;
            }

            std::unique_lock<std::mutex> lock(_mtx);
            auto iter = _connections.find(id);
            if(iter == _connections.end())
            {
                return false;
            }
            *out = iter->second;
            return true;
        }
    private:
        std::unordered_map<std::string, Connection> _connections;
        std::mutex _mtx;
    };
}
