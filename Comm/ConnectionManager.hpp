#pragma once
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <mutex>
#include "../Comm/Socket.hpp"

namespace NS_CONNECTIOM_MANAGER_MODULE
{
    using namespace NS_SOCKET_MODULE;
    class Connection
    {
    public:
        Connection(const std::string& id, const InetAddr& addr)
            :_id(id)
            ,_addr(addr)
        {
        }


    private:
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
                _connections.emplace(id, addr);
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
