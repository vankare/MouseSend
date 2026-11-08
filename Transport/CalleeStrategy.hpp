#pragma once
#include <climits>
#include <set>
#include <functional>
#include <cstdint>
#include "CallerStrategy.hpp"
#include "Connection.hpp"
#include "ServerStrategy.hpp"

namespace NS_STRATEGY_MODULE
{
    using handler_data_t = std::function<void(std::vector<char>)>;
    using namespace NS_CONNECTION_MODULE;
    class CalleeStrategy : public ServerStrategy
    {
    public:
        /*
         * signal_ip: 信令服务器ip
         * signal_port: 信令服务器监听端口
         * handler_data: 处理数据方法 
         * callee_port: 自己监听端口 
         */
        CalleeStrategy(const std::string& self_id, const std::string& signal_ip, uint16_t signal_port, handler_data_t handler_data, uint16_t callee_port = 0)
            :ServerStrategy(callee_port)
            ,_signal_conn("signal", this->GetSockfd(), signal_ip, signal_port)
            ,_handler_data(handler_data)
            ,_self_id(self_id)
        {  }

        Message Answer(Message in_msg, const InetAddr& peer_addr) override
        {
            Type type = static_cast<Type>(in_msg.GetType());
            if(!(type == SYN || type == FIN))
            {
                return Message(ERROR, 0, std::vector<char>());
            }
            if(type == FIN)
            {
                _max_seq = in_msg.GetSeq();
            }
            auto&& payload =  in_msg.GetPayload();
            uint64_t seq = in_msg.GetSeq();

            _seqs.insert(seq);
            size_t new_size = std::max(seq+ payload.size(), _data.size());
            _data.resize(new_size);

            memcpy(_data.data() + seq, payload.data(), payload.size());

            if(IsFinish())
            {
                Stop();
                _handler_data(_data);
                return Message(FIN, seq, std::vector<char>());
            }
            return Message(Type::ACK, seq, std::vector<char>());
        }

        /*
         * 连接服务器
         */
        bool ConnectSignal()
        {
            Message connect_msg(CONNECT, 0, std::vector<char>(_self_id.begin(), _self_id.end()));
            std::vector<char> conn_vec;
            connect_msg.Serializate(&conn_vec);

            _signal_conn.RecvTimeOut(g_timeout);
            for(int i = 0; i < g_max_fails; ++i)
            {
                _signal_conn.Send(conn_vec);
                std::vector<char> recv_vec;
                int n = _signal_conn.Recv(&recv_vec);
                {
                    if(n > 0)
                    {
                        Message msg;
                        msg.Deserializate(recv_vec);
                        if(msg.GetType() == CONNECT)
                        {
                            LOG(LogLevel::INFO) << "成功连上信令服务器";
                            return true;
                        }
                    }
                }
            }
            LOG(LogLevel::ERRNO) << "连接信令服务器失败";
            return false;
        }
    private:
        bool IsFinish()
        {
            if(_max_seq == ULLONG_MAX || _seqs.empty())
            {
                return false;
            }
            
            if(_seqs.size() == 1 && _max_seq != ULLONG_MAX)
            {
                return true;
            }

            auto iter = _seqs.begin();
            auto next = std::next(iter);
            uint64_t diff = *next - *iter;
            for(; next != _seqs.end(); ++next, ++iter)
            {
                if(*next - *iter != diff)
                {
                    return false;
                }
            }
            return true;
        }
    private: 
        std::set<uint64_t> _seqs;
        handler_data_t _handler_data;        // 处理数据方法(外部注册)
        uint64_t _max_seq = ULLONG_MAX;
        std::string _self_id;                // 自己id
        Connection _signal_conn;
    };
}
