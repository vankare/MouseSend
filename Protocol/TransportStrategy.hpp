#include "../Connection/ConnectionManager.hpp"
#include "Protocol.hpp"
#include <atomic>
#include <cerrno>
#include <thread>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace NS_PROTOCOL_MODULE
{
    const uint64_t g_chunk_size = 4096;
    using namespace NS_CONNECTIOM_MODULE;
    struct PendingPacket
    {
        PendingPacket(std::vector<char>&& data)
            :_tries(0)
            ,_last_active_time(std::chrono::steady_clock::now())
            ,_data(data)
        {  }

        PendingPacket(const std::vector<char>& data)
            :_tries(1)
            ,_last_active_time(std::chrono::steady_clock::now())
            ,_data(data)
        {  }

        std::chrono::time_point<std::chrono::steady_clock> _last_active_time;
        std::atomic_uint32_t _tries;
        std::vector<char> _data;
    };

    class ClientStrategy
    {
    private:
        // 根据大小分片
        std::vector<std::vector<char>> MakeChunk(const std::vector<char>& data, uint64_t chunk_size = g_chunk_size)
        {
            std::vector<std::vector<char>> chunks;
            for(uint64_t i = 0; i < data.size(); i += chunk_size)
            {
                uint64_t end = std::min(i + chunk_size, data.size());
                chunks.push_back( std::vector<char>(data.begin() + i, data.begin() + i + chunk_size));
            }
            return chunks;
        }

    public:
        ClientStrategy(const Connection& conn)
            :_conn(conn)
        {  }

        ~ClientStrategy()
        {  }

        bool Send(const std::vector<char>& data)
        {
            // 1. 分片
            auto chunks = MakeChunk(data);

            runing++;
            std::thread recv_thread(
                [this]()
                {
                    this->RecvLoop();
                }
            );
            for(uint32_t i = 0; i < chunks.size(); i += chunks[0].size())
            {
                // 2. 构造请求
                uint64_t seq = chunks[i].size();
                Message msg(SYN, seq, chunks[i]);

                // 3. 序列化
                std::vector<char> msg_vec;
                msg.Serializate(&msg_vec);
                // 4. 发送
                int n = _conn._sockfd->Send(msg_vec, _conn._addr);
                if(n <= 0)
                {
                    LOG(LogLevel::ERRNO) << "updsocket: 发送失败";
                }
                
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    // 5. 将数据包和序列号管理起来
                    _packets.emplace(seq, PendingPacket(msg_vec));
                }

            }
            runing--;
            return true;
        }

        void RecvLoop()
        {
            std::vector<Message> datas;
            while(runing)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                std::vector<char> recv_buf;
                errno = 0;
                int n = _conn._sockfd->Recv(&recv_buf, nullptr);
                if(n >= 0)
                {
                    Message message;
                    message.Deserializate(recv_buf);
                    if(message.GetType() != ACK)
                    {
                        continue;
                    }
                    datas.push_back(message);
                    uint64_t seq = message.GetSeq();
                    _packets.erase(seq);
                }
                else
                {

                    // 检查是否有超时的
                    std::unique_lock<std::mutex> lock(_mtx);
                    for(auto& [seq, packet] : _packets)
                    {
                        // 没有超时, 并且没有达到最大失败次数
                        if(std::chrono::steady_clock::now() - packet._last_active_time < g_timeout 
                                && packet._tries < g_max_fails)
                        {
                            continue;
                        }
                        // 超时了，但是没有达到最大失败次数
                        else if(packet._tries < g_max_fails) 
                        {
                            packet._tries++;
                            _conn._sockfd->Send(packet._data, _conn._addr);
                            packet._last_active_time = std::chrono::steady_clock::now();
                        }
                        // 超时，并且达到最大失败次数
                        else
                        {
                            LOG(LogLevel::FATAL) << "数据报遗失";
                            return;
                        }
                    }
                }
            }
        }

    private:
        std::unordered_map<uint64_t, PendingPacket> _packets;
        Connection _conn;
        std::mutex _mtx;
        std::atomic_uint32_t runing;
    };
}
