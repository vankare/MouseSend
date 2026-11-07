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
        bool Relation(const std::string& sender, const std::string& recver)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if(_connections.find(sender) == _connections.end() || _connections.find(recver) == _connections.end())
            {
                return false;
            }

            _sender2recver.emplace(sender, recver);
            _recver2sender.emplace(recver, sender);
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
            // id为sender
            if(_sender2recver.find(id) != _sender2recver.end())
            {
                other = _sender2recver[id];
                _sender2recver.erase(id);
                _recver2sender.erase(other);
            }
            // id为recver
            else if(_recver2sender.find(id) != _recver2sender.end())
            {
                other = _recver2sender[id];
                _recver2sender.erase(id);
                _sender2recver.erase(other);
            }
            _connections.erase(id);

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
            if(_sender2recver.find(id) != _sender2recver.end())
            {
                return _sender2recver[id];
            }
            else if(_recver2sender.find(id) != _recver2sender.end())
            {
                return _recver2sender[id];
            }
            else
            {
                return std::string();
            }
        }
        
    private:
        std::unordered_map<std::string, Connection> _connections;
        std::mutex _mtx;
        std::unordered_map<std::string, std::string> _sender2recver;
        std::unordered_map<std::string, std::string> _recver2sender;
    };
}
