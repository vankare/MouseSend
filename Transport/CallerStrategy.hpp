#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "../Net/Logger.hpp"
#include "../Message/Message.hpp"
#include "Connection.hpp"

namespace NS_STRATEGY_MODULE
{
    using namespace NS_CONNECTION_MODULE;
    using namespace NS_LOGGER_MUDULE;
    using namespace NS_MESSAGE_MODULE;
    const char g_ip_port_sep = ':';    // 例如: "127.0.0.1:8888"
    const char g_two_id_sep = '&';

    const std::chrono::milliseconds g_timeout(5000); // // 超时, 默认5000毫秒
    const int g_max_fails = 3;            // // 最大失败次数
    const uint64_t g_chunk_size = 4096;   // 分片大小
    const uint16_t g_win_size = 4;        // 窗口大小
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
                i += chunk_size;
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
                messages.emplace_back(FIN, (messages.back().GetSeq()) + chunks[0].size(), std::move(chunks.back()));
            }

            return messages;
        }

    public:
        CallerStrategy(const std::string& self_id, const std::string& signal_ip, uint16_t signal_port)
            :_signal_conn("signal", {signal_ip, signal_port})
            ,_self_id(self_id)
            { 
                 _signal_conn.RecvTimeOut(g_timeout);
            }

        bool StartSend(const std::vector<char>& data)
        {
            // 1. 分片
            auto chunks = MakeChunk(data);

            // 2. 包装成请求
            auto messages = MakeRequest(std::move(chunks));

            // 3. 创建窗口(下标: 包)
            std::unordered_map<uint64_t, PendingPacket> window;

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
                    for(auto& [seq, packet] : window)
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        int n = _conn.Send(packet._send_buf);
                        if(n < 0)
                        {
                            LOG(LogLevel::WARNING) << "尝试一次发送失败" << n;
                        }
                        else
                        {
                            LOG(LogLevel::DEBUG) << "发送数据报成功, seq: " << seq;
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
                                if(recv_msg.GetSeq() == seq && (recv_msg.GetType() == ACK || recv_msg.GetType() == FIN))
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
                                    int n = _conn.Send(packet._send_buf);
                                    if(n > 0)
                                    {
                                        LOG(LogLevel::WARNING) << "超时重发一个数据报, seq: " << seq;
                                    }
                                    continue;
                                }
                                // 没有达到最大失败次数，没有超时
                                else
                                {
                                    packet._tries++;
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

        /*
         * 连接服务器
         */
        bool ConnectSignal()
        {
            Message connect_msg(CONNECT, 0, std::vector<char>(_self_id.begin(), _self_id.end()));
            std::vector<char> connect_vec;
            connect_msg.Serializate(&connect_vec);
            for(int i = 0; i < g_max_fails; ++i)
            {
                _signal_conn.Send(connect_vec);
                std::vector<char> recv_vec;
                int n = _signal_conn.Recv(&recv_vec);
                {
                    if(n > 0)
                    {
                        Message msg;
                        msg.Deserializate(recv_vec);
                        if(msg.GetType() == CONNECT)
                        {
                            LOG(LogLevel::INFO) << "成功连接上信令服务器";
                            return true;
                        }
                    }
                }
            }
            LOG(LogLevel::ERRNO) << "无法连接上信令服务器";
            return false;
        }

        /*
         * 获取callee地址
         * callee_id: 实际接收数据的对端服务器id
         */
        bool GetCalleeAddr(const std::string& callee_id)
        {
            // 构造报文有效载荷: self_id&callee_id
            std::string payload = _self_id + g_two_id_sep + callee_id;
            Message two_id_msg(SIGNAL, 0, std::vector<char>(payload.begin(), payload.end()));
            std::vector<char> id_vec;
            two_id_msg.Serializate(&id_vec);


            _signal_conn.RecvTimeOut(g_timeout);
            for(int i = 0; i < g_max_fails; ++i)
            {
                _signal_conn.Send(id_vec);
                std::vector<char> from_server;
                int n = _signal_conn.Recv(&from_server);
                {
                    if(n > 0)
                    {
                        Message msg;
                        msg.Deserializate(from_server);
                        if(msg.GetType() == SIGNAL)
                        {
                            std::vector<char> callee_addr = msg.GetPayload();
                            auto token_pos = std::find(callee_addr.begin(), callee_addr.end(), g_ip_port_sep);
                            std::string callee_ip(callee_addr.begin(), token_pos);
                            std::string callee_port(std::string(token_pos + 1, callee_addr.end()));
                            _conn = Connection(callee_id, {callee_ip, static_cast<uint16_t>(std::stoul(callee_port))});
                            LOG(LogLevel::INFO) << "获取callee地址" << callee_ip + ":" << callee_port;
                            return true;
                        }
                    }
                }
            }
            LOG(LogLevel::ERRNO) << "无法获取callee地址";
            return false;
        }
    private:
        Connection _conn;        // 对端连接
        Connection _signal_conn; // 信令服务器
        std::string _self_id;    // 自己的id
        std::mutex _mtx;
    };
}
