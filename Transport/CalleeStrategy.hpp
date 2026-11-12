#pragma once
#include <climits>
#include <set>
#include <functional>
#include <cstdint>
#include "Connection.hpp"
#include "ServerStrategy.hpp"
#include "ClientStrategy.hpp"

namespace NS_STRATEGY_MODULE
{
    using handler_data_t = std::function<void(std::vector<char>)>;
    using namespace NS_CONNECTION_MODULE;

    class CalleeStrategy : public ServerStrategy ,public ClientStrategy   /* 基类初始化顺序由继承时声明决定 */
    {
    public:
        /*
         * signal_ip: 信令服务器ip
         * signal_port: 信令服务器监听端口
         * handler_data: 处理数据方法 
         * callee_port: 自己监听端口 
         *
         * 注意: 这里的初始化列表顺序不能改变
         */
        CalleeStrategy(const std::string& self_id, const std::string& signal_ip, uint16_t signal_port, handler_data_t handler_data, uint16_t callee_port = 0)
            :ServerStrategy(callee_port)
            ,ClientStrategy("signal", self_id, signal_ip, signal_port, _svr.Get())
            ,_handler_data(handler_data)
        {  }

        Message Answer(Message in_msg, const InetAddr& peer_addr) override
        {
            Type type = static_cast<Type>(in_msg.GetType());
            Message ret_msg(ERROR, 0, std::vector<char>());
            switch(type)
            {
                case FIN:
                {
                    _max_seq = in_msg.GetSeq();
                }
                case SYN:
                {

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
                    ret_msg =  Message(Type::ACK, seq, std::vector<char>());
                }
                default:
                    break;
            }
            return ret_msg;
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
        Connection _signal_conn;
    };
}
