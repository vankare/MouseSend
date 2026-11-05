#pragma once
#include "ServerStrategy.hpp"
#include "Connection.hpp"
#include "ConnectionManager.hpp"

namespace NS_STRATEGY_MODULE
{
    using namespace NS_CONNECTIOM_MODULE;
    class SignalStrategy : public ServerStrategy
    {
    public:
        SignalStrategy(handler_data_t handler_data, uint16_t port = 0)
            :ServerStrategy(handler_data, port)
        {  }

        Message Answer(Message in_msg) override
        {
            Message ret_msg(ACK, 0, std::vector<char>());
            std::vector<char> data = in_msg.GetPayload();
            switch(in_msg.GetSeq())
            {
                // 建立连接的请求
                case SIGNAL:
                {
                    auto peer_addr = HandlerSignal(std::string(data.begin(), data.end()));
                    ret_msg.SetType(SIGNAL);
                    ret_msg.SetPayload(std::vector<char>(peer_addr.begin(), peer_addr.end()));
                    break;
                }
                // 断开连接的请求
                case FIN:
                {
                    auto self_id = in_msg.GetPayload();
                    if(self_id.empty())
                    {
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    bool success = HandlerFin(std::string(self_id.begin(), self_id.end()));
                    if(!success)
                    {
                        ret_msg.SetType(ERROR);
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
        // 传入一个对方id，返回对方地址报文。
        std::string HandlerSignal(const std::string& peer_id)
        {
            Connection peer_conn;
            _cm.GetConnById(peer_id, &peer_conn);
            return peer_conn._addr.ToString();
        }

        // 传入自己id, 能够删除对端id记录和断开关联
        bool HandlerFin(const std::string& self_id)
        {
            _cm.DelConnection(self_id);
            return true;
        }

    private:
        ConnectionManager _cm;           // 连接管理器
    };
}
