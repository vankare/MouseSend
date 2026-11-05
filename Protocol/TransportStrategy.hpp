#pragma once
#include "../Server/Server.hpp"
#include "../Comm/Logger.hpp"
#include "../Connection/ConnectionManager.hpp"
#include "Protocol.hpp"
#include <cerrno>
#include <climits>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <functional>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <set>

namespace NS_PROTOCOL_MODULE
{
    using handler_data_t = std::function<std::vector<char>(std::vector<char>)>;
    using namespace NS_CONNECTIOM_MODULE;
    using namespace NS_SERVER_MODULE;

    const uint64_t g_init_recv_buf_size = 40960;   // 预开辟接收缓冲区大小
    const uint64_t g_chunk_size = 4096;   // 分片大小
    const uint16_t g_win_size = 4;        // 窗口大小
    const int g_max_fails = 3;            // // 最大失败次数
    const std::chrono::milliseconds g_timeout(5000); // // 超时, 默认5000毫秒
    const char g_token = '&';
    struct PendingPacket
    {
        PendingPacket(std::vector<char>&& send_buf)
            :_tries(0)
            ,_last_active_time(std::chrono::steady_clock::now())
            ,_send_buf(send_buf)
        {  }

        PendingPacket(const std::vector<char>& send_buf)
            :_tries(1)
            ,_last_active_time(std::chrono::steady_clock::now())
            ,_send_buf(send_buf)
        {  }

        std::chrono::time_point<std::chrono::steady_clock> _last_active_time;
        uint32_t _tries;
        std::vector<char> _send_buf;
    };

    // 调用者(实际发送数据的消息)
    class CallerStrategy
    {
    private:
        // 根据大小分片
        std::vector<std::vector<char>> MakeChunk(const std::vector<char>& data, uint64_t chunk_size = g_chunk_size)
        {
            std::vector<std::vector<char>> chunks;
            for(uint64_t i = 0; i < data.size(); )
            {
                uint64_t end = std::min(i + chunk_size, data.size());

                chunks.push_back( std::vector<char>(data.begin() + i, data.begin() + end));
                i += end;
            }
            return chunks;
        }

        std::vector<Message> MakeRequest(std::vector<std::vector<char>>&& chunks)
        {
            std::vector<Message> messages;
            for(uint64_t i = 0; i < chunks.size() - 1; ++i)
            {
                messages.emplace_back(SYN, i * chunks[0].size(), std::move(chunks[i]));
            }
            // 特殊处理最后一个包
            if(chunks.size() == 1)
            {
                messages.emplace_back(FIN, 0, std::move(chunks[0]));

            }
            else
            {
                messages.emplace_back(FIN,  (messages.back().GetSeq()) + chunks[0].size(), std::move(chunks.back()));
            }

            return messages;
        }

    public:
        CallerStrategy(const Connection& conn)
            :_conn(conn)
        {  }

        bool StartSend(const std::vector<char>& data)
        {
            // 1. 分片
            auto chunks = MakeChunk(data);

            // 2. 包装成请求
            auto messages = MakeRequest(std::move(chunks));

            // 3. 创建窗口(下标: 包)
            std::unordered_map<uint64_t, PendingPacket> window;
            std::mutex _mtx;

            std::vector<char> recv_vec;

            // 4. 设置套接字超时时间
            _conn.RecvTimeOut(g_timeout);
            LOG(LogLevel::INFO) << "分片、封装报文完成, 即将开始发送工作"; 
            for(uint64_t i = 0; i < messages.size(); ++i)
            {
                // 5. 当窗口大小小于规定大小时，入窗口
                std::vector<char> send_vec;
                messages[i].Serializate(&send_vec);
                window.emplace(messages[i].GetSeq(), PendingPacket(std::move(send_vec)));

                // 6. 当窗口大小等于规定大小或者是最后一个请求，发送全部请求
                if(window.size() == g_win_size || i == messages.size() - 1)
                {
                    for(auto& [_, packet] : window)
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        int n = _conn.Send(packet._send_buf);
                        if(n < 0)
                        {
                            LOG(LogLevel::WARNING) << "尝试一次发送失败" << n;
                        }
                        else
                        {
                            LOG(LogLevel::DEBUG) << "发送数据报成功, seq:";
                        }
                    }

                    while(!window.empty())
                    {
                        int n = _conn.Recv(&recv_vec);
                        // 7. 接收到响应，出窗口
                        if(n > 0)
                        {
                            Message recv_msg;
                            recv_msg.Deserializate(recv_vec);

                            for(auto& [seq, packet] : window)
                            {
                                if(recv_msg.GetSeq() == seq && recv_msg.GetType() == ACK)
                                {
                                    LOG(LogLevel::INFO) << "收到一个数据报";
                                    window.erase(seq);
                                    break;
                                }
                            }
                        }
                        // 8. 没有接收到响应, 判断是否超时，是否重发
                        else
                        {
                            for(auto& [seq, packet] : window)
                            {
                                // 达到最大失败次数，判定文件传输失败
                                if(packet._tries >= g_max_fails)
                                {
                                    LOG(LogLevel::FATAL) << "失败达到最大次数，文件传输失败";
                                    return false;
                                }
                                // 没有达到最大失败次数，超时
                                else if(std::chrono::steady_clock::now() - packet._last_active_time > g_timeout)
                                {
                                    packet._tries++;
                                    packet._last_active_time = std::chrono::steady_clock::now();
                                    continue;
                                }
                                // 没有达到最大失败次数，没有超时
                                else
                                {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
    private:
        Connection _conn;     // 对端连接
    };

    // 被调用者(实际接收数据的机器)
    class ServerStrategy
    {
    public:
        /*
         * handler_t 处理套接字读到的数据
         * handler_msg_t 处理协议解析后的数据
         * handler_data_t 处理业务数据
         */

        ServerStrategy(handler_data_t handler_data, uint16_t port = 0)
            :_svr(port)
            ,_handler_data(handler_data)
        {  
            _svr.Register(
                [this](std::vector<char> req) -> std::vector<char>
                {
                    return this->_protocol.ParseProtocol(req);
                }
            );

            _protocol.Register(
                [this](const Message& req) -> Message
                {
                    std::vector<char> resp_vec = this->_handler_data(req.GetPayload());
                    return Message(ACK, req.GetSeq(), resp_vec);
                }
            );

            _data.reserve(g_init_recv_buf_size);

            LOG(LogLevel::DEBUG) << "注册处理数据方法成功";
        }

        void StartRecv()
        {
            _svr.Loop();
        }

        virtual Message Answer(Message in_msg) = 0;
    protected:
        Server _svr;                         // 基础服务器
        Protocol _protocol;                  // 协议
        handler_data_t _handler_data;        // 处理数据方法(外部注册)
        std::vector<char> _data;             // 接收到的数据
    };

    class CalleeStrategy : public ServerStrategy
    {
    public:
        CalleeStrategy(handler_data_t handler_data, uint16_t port = 0)
            :ServerStrategy(handler_data, port)
        {  }

        Message Answer(Message in_msg) override
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
                return Message(FIN, seq, std::vector<char>());
            }
            return Message(Type::ACK, seq, std::vector<char>());
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
        uint64_t _max_seq = ULLONG_MAX;
    };

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
