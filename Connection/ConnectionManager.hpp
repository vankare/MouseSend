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

    class ConnectionManager
    {
    public:
        // 添加连接记录
        void AddConnection(const std::string& id, const InetAddr& addr)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) != _connections.end())
            {
                _connections.emplace(id, Connection(id, addr));
            }
        }

        // 关联两个连接
        bool Relation(const std::string& id1, const std::string& id2)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id1) == _connections.end() || _connections.find(id2) == _connections.end())
            {
                return false;
            }

            _p2p.emplace(id1, id2);
            _p2p.emplace(id2, id1);
            return true;
        }

        // 删除连接记录，并断开关联
        void DelConnection(const std::string id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) == _connections.end())
            {
                return;
            }

            std::string other;
            if(_p2p.find(id) != _p2p.end())
            {
                other = _p2p[id];
                _p2p.erase(other);
                _connections.erase(other);
            }

            _p2p.erase(id);
            _connections.erase(id);
        }

        // 根据id找连接
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

        // 根据id找对端
        bool GetPeerById(const std::string& id, Connection* out)
        {
            if(out == nullptr)
            {
                return false;
            }

            std::unique_lock<std::mutex> lock(_mtx);
            auto iter = _p2p.find(id);
            if(iter == _p2p.end())
            {
                return false;
            }

            *out = _connections[iter->second];
            return true;
        }
        
    private:
        std::unordered_map<std::string, Connection> _connections;
        std::mutex _mtx;
        std::unordered_map<std::string, std::string> _p2p;
    };
}
