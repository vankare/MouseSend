#pragma once
#include "../Config/Config.hpp"
#include "Connection.hpp"
#include "../Net/InetAddr.hpp"
#include <chrono>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <mutex>

namespace NS_CONNECTION_MODULE
{
    using namespace NS_UTIL_MODULE;
    using namespace NS_CONFIG_MODULE;

    template <class F>
    struct ScopeGuard 
    {
        F f;
        bool active = true;
        explicit ScopeGuard(F fn) : f(std::move(fn)) {}
        ~ScopeGuard() { if (active) f(); }
        void dismiss() noexcept { active = false; }
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
    };
#define SCOPE_CONCAT_(a, b) a##b
#define SCOPE_CONCAT(a, b)  SCOPE_CONCAT_(a, b)
#define SCOPE_EXIT(...) \
    auto SCOPE_CONCAT(scope_guard_, __LINE__) = ScopeGuard([&]() { __VA_ARGS__; })

    class ConnectionManager
    {
    public:
        // 添加连接记录
        bool AddConnection(const std::string& id, const InetAddr& addr)
        {
            SCOPE_EXIT(CheckExpired());
            std::unique_lock<std::mutex> lock(_mtx);

            if(_connections.find(id) != _connections.end())
            {
                // 已连接客户端的请求
                // 不能直接调DelConnection, 会死锁
                DelConnCore(id);
            }
            Connection new_conn(id, addr);
            new_conn._last_active_time = std::chrono::steady_clock::now();
            _connections.emplace(id, std::move(new_conn));
            return true;
       }

        // 关联两个连接
        bool Relation(const std::string& self_id, const std::string& peer_id)
        {
            SCOPE_EXIT(CheckExpired());
            std::unique_lock<std::mutex> lock(_mtx);

            if(_connections.find(self_id) == _connections.end() || _connections.find(peer_id) == _connections.end())
            {
                return false;
            }

            _connections[self_id]._last_active_time = std::chrono::steady_clock::now();
            _connections[peer_id]._last_active_time = std::chrono::steady_clock::now();
            _p2p.emplace(self_id, peer_id);
            _p2p.emplace(peer_id, self_id);
            return true;
        }

        // 删除连接记录，并断开关联
        bool DelConnection(const std::string id)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            return DelConnCore(id);
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
        void CheckExpired()
        {
            std::vector<std::string> to_delete;
            for(auto& [id1, _] : _p2p)
            {
                if(std::chrono::steady_clock::now() - _connections[id1]._last_active_time > ServerConfig::_connection_exired_timeout)
                {
                    // 过期连接, 不能删除, 会导致迭代器失效
                    to_delete.push_back(id1);
                }
            }
            for(auto& id : to_delete)
            {
                DelConnection(id);
            }
        }

        // 不加锁，留给上面加锁
        bool DelConnCore(const std::string& id)
        {
            if(_connections.find(id) == _connections.end())
            {
                return false;
            }

            std::string other;

            if(_p2p.find(id) != _p2p.end())
            {
                other = _p2p[id];
                _p2p.erase(id);
                _p2p.erase(other);
                _connections.erase(id);
            }

            return true;

        }
    private:
        std::unordered_map<std::string, Connection> _connections;
        std::mutex _mtx;
        std::unordered_map<std::string, std::string> _p2p;
    };
}
