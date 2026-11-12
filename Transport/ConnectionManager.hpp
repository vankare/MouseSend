#pragma once
#include "Connection.hpp"
#include "../Net/InetAddr.hpp"
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <mutex>

namespace NS_CONNECTION_MODULE
{

    class ConnectionManager
    {
    public:
        // 添加连接记录
        bool AddConnection(const std::string& id, const InetAddr& addr)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) == _connections.end())
            {
                _connections.emplace(id, Connection(id, addr));
                return true;
            }
            return false;
        }

        // 关联两个连接
        bool Relation(const std::string& self_id, const std::string& peer_id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(self_id) == _connections.end() || _connections.find(peer_id) == _connections.end())
            {
                return false;
            }

            _p2p.emplace(self_id, peer_id);
            return true;
        }

        // 删除连接记录，并断开关联
        bool DelConnection(const std::string id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(id) == _connections.end())
            {
                return false;
            }

            std::string other;
            // id为self
            if(_p2p.find(id) != _p2p.end())
            {
                other = _p2p[id];
                _p2p.erase(id);
                _p2p.erase(other);
                _connections.erase(id);
                _connections.erase(other);
            }

            return true;
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

        // 根据id找对端id
        std::string GetPeerById(const std::string& id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_p2p.find(id) != _p2p.end())
            {
                return _p2p[id];
            }
            else
            {
                return std::string();
            }
        }
        
    private:
        std::unordered_map<std::string, Connection> _connections;
        std::mutex _mtx;
        std::unordered_map<std::string, std::string> _p2p;
    };
}
