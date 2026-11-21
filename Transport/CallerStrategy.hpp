#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include "../Net/Logger.hpp"
#include "../Message/Message.hpp"
#include "Connection.hpp"
#include "ClientStrategy.hpp"
#include "../Config/Config.hpp"

namespace NS_STRATEGY_MODULE
{
    using namespace NS_CONNECTION_MODULE;
    using namespace NS_LOGGER_MUDULE;
    using namespace NS_MESSAGE_MODULE;
    using namespace NS_CONFIG_MODULE;
    const char g_ip_port_sep = ':';    // 例如: "127.0.0.1:8888"

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
    class CallerStrategy : public ClientStrategy
    {
    private:
        // 根据大小分片
        std::vector<std::vector<char>> MakeChunk(const std::vector<char>& data, uint64_t chunk_size = CallerConfig::_send_chunk_size)
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

        std::vector<Message> MakeRequest(std::vector<std::vector<char>>&& chunks, bool isend)
        {
            std::vector<Message> messages;
            for(uint64_t i = 0; i < chunks.size(); ++i)
            {
                messages.emplace_back(SYN, i * chunks[0].size(), std::move(chunks[i]));
            }
            // 特殊处理最后一个包
            if(isend)
            {
                messages.back().SetType(FIN);
            }

            return messages;
        }

    public:
        CallerStrategy(const std::string& self_id, const std::string& signal_ip, uint16_t signal_port)
            :ClientStrategy("signal", self_id, signal_ip, signal_port)
            ,_self_id(self_id)
            { 
                 _signal_conn.RecvTimeOut(CallerConfig::_send_recv_timeout);
            }

        // 发送data数据, isend: 是否最后一段数据
        bool StartSend(const std::vector<char>& data, bool isend)
        {
            if(_phase != PHASE_FOUR)
            {
                // 还没有完成四次握手
                return false;
            }

            // 1. 分片
            auto chunks = MakeChunk(data);

            // 2. 包装成请求
            auto messages = MakeRequest(std::move(chunks), isend);

            // 3. 创建窗口(下标: 包)
            std::unordered_map<uint64_t, PendingPacket> window;

            std::vector<char> recv_vec;

            // 4. 设置套接字超时时间
            _peer_conn.RecvTimeOut(CallerConfig::_send_recv_timeout);
            LOG(LogLevel::INFO) << "分片、封装报文完成, 即将开始发送工作"; 
            for(uint64_t i = 0; i < messages.size(); ++i)
            {
                // 5. 当窗口大小小于规定大小时，入窗口
                std::vector<char> send_vec;
                messages[i].Serializate(&send_vec);
                window.emplace(messages[i].GetSeq(), PendingPacket(std::move(send_vec)));

                // 6. 当窗口大小等于规定大小或者是最后一个请求，发送全部请求
                if(window.size() == CallerConfig::_send_win_size || i == messages.size() - 1)
                {
                    for(auto& [seq, packet] : window)
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        int n = _peer_conn.Send(packet._send_buf);
                        if(n < 0)
                        {
                            LOG(LogLevel::WARNING) << "尝试一次发送失败" << n;
                        }
                    }

                    while(!window.empty())
                    {
                        int n = _peer_conn.Recv(&recv_vec);
                        // 7. 接收到响应，出窗口
                        if(n > 0)
                        {
                            Message recv_msg;
                            recv_msg.Deserializate(recv_vec);

                            for(auto& [win_seq, packet] : window)
                            {
                                if(recv_msg.GetSeq() == win_seq && (recv_msg.GetType() == ACK || recv_msg.GetType() == FIN))
                                {
                                    LOG(LogLevel::DEBUG) << "成功发送一个报文 seq: " << win_seq; 
                                    window.erase(win_seq);
                                    // 一定要break, 防止迭代器失效
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
                                if(packet._tries >= CallerConfig::_max_fails)
                                {
                                    LOG(LogLevel::FATAL) << "失败达到最大次数，文件传输失败";
                                    return false;
                                }
                                // 没有达到最大失败次数，超时
                                else if(std::chrono::steady_clock::now() - packet._last_active_time > CallerConfig::_send_recv_timeout)
                                {
                                    packet._tries++;
                                    packet._last_active_time = std::chrono::steady_clock::now();
                                    int n = _peer_conn.Send(packet._send_buf);
                                    if(n > 0)
                                    {
                                        LOG(LogLevel::WARNING) << "超时重发一个数据报, seq: " << seq;
                                    }
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

        // 打洞: caller打洞，需要等待回复。
        bool HolePunching() override
        {
            Message hole_msg(PUNCH, 0, std::vector<char>());
            std::vector<char> hole_vec;
            hole_msg.Serializate(&hole_vec);
            bool ok = SendAndRecv(
                hole_vec, 
                PUNCH, 
                _peer_conn,
                [](std::vector<char>){},
                10
                );
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

        // 四次握手, 发起第一次, 接收第二次, 发起第三次, 接收第四次
        bool RequestHandshake()
        {
            // 发送第一次握手请求 和 接收第二次握手请求
            Message msg_1st(HANDSHAKE1st, 0, std::vector<char>(_sfile_name.begin(), _sfile_name.end()));
            std::vector<char> buf_1st;
            msg_1st.Serializate(&buf_1st);
            bool ok = SendAndRecv(
                buf_1st,
                HANDSHAKE2nd,
                _peer_conn, 
                [](std::vector<char>){}
            );
            if(ok)
            {
                _phase = PHASE_TWO;
            }

            // 发送第三次握手请求 和 接收第四次握手请求
            std::string file_size = std::to_string(FileUtil::FileSize(_sfile_name));
            Message msg_3rd(HANDSHAKE3rd, 0, std::vector<char>(file_size.begin(), file_size.end()));
            std::vector<char> buf_3rd;
            msg_3rd.Serializate(&buf_3rd);
            ok &= SendAndRecv(
                buf_3rd,
                HANDSHAKE4th,
                _peer_conn, 
                [](std::vector<char>){}
            );
            if(ok)
            {
                _phase = PHASE_FOUR;
            }

            return ok;
        }

        std::string& SendFileName()
        {
            return _sfile_name;
        }

    private:
        std::string _self_id;    // 自己的id
        std::mutex _mtx;
        std::string _sfile_name; // 发送的文件名
    };
}
