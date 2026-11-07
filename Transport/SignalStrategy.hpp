#pragma once
#include "ServerStrategy.hpp"
#include "Connection.hpp"
#include "ConnectionManager.hpp"
#include <algorithm>
#include <sys/socket.h>

namespace NS_STRATEGY_MODULE
{
    using namespace NS_CONNECTION_MODULE;

    const char g_token = '&';
    class SignalStrategy : public ServerStrategy
    {
    public:
        SignalStrategy(uint16_t port = 0)
            :ServerStrategy(port)
        {  }

        Message Answer(Message in_msg, const InetAddr& addr) override
        {
            Message ret_msg(ACK, 0, std::vector<char>());
            std::vector<char> data = in_msg.GetPayload();
            switch(in_msg.GetType())
            {
                // 处理获取对方连接的请求
                case SIGNAL:
                {
                    auto token_pos = std::find(data.begin(), data.end(), g_token);
                    if(token_pos == data.end())
                    {
                        LOG(LogLevel::DEBUG) << "处理获取对方地址请求失败: 不存在id分隔符";
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    std::string peer_addr;
                    std::string self_id(data.begin(), token_pos);
                    std::string peer_id(token_pos + 1, data.end());
                    bool success = HandlerSignal(self_id, peer_id, &peer_addr);
                    if(!success)
                    {
                        LOG(LogLevel::DEBUG) << "处理获取对方地址请求失败: 回调失败" << addr.ToString();
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    ret_msg.SetType(SIGNAL);
                    ret_msg.SetPayload(std::vector<char>(peer_addr.begin(), peer_addr.end()));
                    LOG(LogLevel::DEBUG) << "成功处理一个获取对方地址请求";
                    break;
                }

                // 处理连接信令服务器的请求
                case CONNECT:
                {
                    std::string id(data.begin(), data.end());
                    bool success = HandlerConnect(id, addr);
                    if(!success)
                    {
                        ret_msg.SetType(ERROR);
                        LOG(LogLevel::DEBUG) << "处理连接请求失败: peer_addr" << addr.ToString();
                        break;
                    }
                    ret_msg.SetType(CONNECT);
                    LOG(LogLevel::DEBUG) << "成功接收到一个连接请求";
                    break;
                }
                // 处理断开连接的请求
                case FIN:
                {
                    auto id = in_msg.GetPayload();
                    if(id.empty())
                    {
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    bool success = HandlerFin(std::string(id.begin(), id.end()));
                    if(!success)
                    {
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    ret_msg.SetType(FIN);
                    break;
                }
                default:
                {
                    ret_msg.SetType(ERROR);
                    break;
                }
            }
            return ret_msg;
        }

    private:
        // 传入两个id, 建立连接, 并返回对方地址
        bool HandlerSignal(const std::string& sender_id, const std::string& recver_id, std::string* const out_addr)
        {
            if(out_addr == nullptr)
            {
                return false;
            }
            Connection recv_conn;
            bool success = _cm.GetConnById(recver_id, &recv_conn);
            success &= _cm.Relation(sender_id, recver_id);
            if(!success)
            {
                return false;
            }

            *out_addr = recv_conn._addr.ToString();
            return true;
        }

        // 传入自己id, 能够删除对端id记录和断开关联
        bool HandlerFin(const std::string& self_id)
        {
            return _cm.DelConnection(self_id);
        }

        // 添加一个新连接 id --> addr
        bool HandlerConnect(const std::string id, const InetAddr& addr )
        {
            return _cm.AddConnection(id, addr);
        }

    private:
        ConnectionManager _cm;           // 连接管理器
    };
}
