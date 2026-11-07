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
    using handler_data_t = std::function<std::vector<char>(std::vector<char>)>;
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
        CalleeStrategy(const std::string& signal_ip, uint16_t signal_port, handler_data_t handler_data, uint16_t callee_port = 0)
            :_signal_conn("signal", {signal_ip, signal_port})
            ,ServerStrategy(callee_port)
            ,_handler_data(handler_data)
        {  }

        Message Answer(Message in_msg, const InetAddr& peer_addr) override
        {
            Type type = static_cast<Type>(in_msg.GetType());
            if(type != SYN || type != FIN)
            {
                return Message(ERROR, 0, std::vector<char>());
            }
            auto&& payload =  in_msg.GetPayload();
            uint64_t seq = in_msg.GetSeq();

            _seqs.insert(seq);
            _data.resize(seq + payload.size());

            memcpy(_data.data() + seq, payload.data(), payload.size());

            if(IsFinish())
            {
                _handler_data(_data);
                return Message(FIN, seq, std::vector<char>());
            }
            return Message(Type::ACK, seq, std::vector<char>());
        }

        /*
         * self_id: 自己id
         * signal_ip: 信令服务器ip
         * signal_port: 信令服务器端口
         */
        bool ConnectSignal(const std::string& self_id)
        {
            Message connect_msg(CONNECT, 0, std::vector<char>(self_id.begin(), self_id.end()));
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
            
            if(_seqs.size() <= 2 && *_seqs.rbegin() == _max_seq)
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
        Connection _signal_conn;
    };
}
