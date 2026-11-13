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
            ,ClientStrategy("signal", self_id, signal_ip, signal_port, _udp_svr.Get())
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
                    break;
                }
                case PUNCH:
                {
                    ret_msg.SetType(PUNCH);
                    break;
                }
                // callee 只能接收到第一次, 第三次请求
                case HANDSHAKE1st:
                {
                    auto&& file_name = in_msg.GetPayload();
                    _rfile_name = std::string(file_name.begin(), file_name.end());
                    ret_msg.SetType(HANDSHAKE2nd);

                    _phase = PHASE_ONE;
                    break;
                }
                case HANDSHAKE3rd:
                {
                    if(_phase != PHASE_ONE)
                    {
                        ret_msg.SetType(ERROR);
                        break;
                    }

                    ret_msg.SetType(HANDSHAKE4th);
                    _phase = PHASE_FOUR;
                    break;
                }
                default:
                    break;
            }
            return ret_msg;
        }

        // 打洞: callee只需向caller发送一批报文, 无需等待回复, 直接进入StartRecv
        bool HolePunching() override
        {
            Message hole_msg(PUNCH, 0, std::vector<char>());
            std::vector<char> hole_vec;
            hole_msg.Serializate(&hole_vec);
            bool ok = false;
            for(int i = 0; i < g_max_fails; ++i)
                ok |= _peer_conn.Send(hole_vec);
            if(ok)
            {
                LOG(LogLevel::INFO) << "打洞成功";
            }
            else
            {
                LOG(LogLevel::INFO) << "打洞失败";
            }

            return ok;
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
        std::string _rfile_name;             // recv到的文件名
    };
}
