#pragma once
#include "../Net/Logger.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unistd.h>
#include <utility>
#include <functional>

namespace NS_MESSAGE_MODULE
{
#define TYPE_LEN 2
#define SEQ_LEN 8
#define MAX_TOTAL_LEN 10240
#define HEAD_LEN (TYPE_LEN + SEQ_LEN)
#define MAX_DATA_LEN (MAX_TOTAL_LEN - HEAD_LEN)

    enum Type : uint16_t
    {
        SYN = 0x01,
        ACK,
        FIN,
        SIGNAL,
        HANDSHAKE1st,
        HANDSHAKE2nd,
        HANDSHAKE3rd,
        ERROR
    };


    using namespace NS_LOGGER_MUDULE;

    class Message
    {
    private:
        // 提取buf下标为offset的内存变量
        template <class T>
        T ReadField(const std::vector<char>& buf, size_t offset) 
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if (offset + sizeof(T) > buf.size())
            {
                LOG(LogLevel::ERRNO) << "读取失败，接收缓冲区太小了";
                return T();
            }
            T v;
            std::memcpy(&v, buf.data() + offset, sizeof(T));
            return v;
        }

        uint16_t ParseType(const std::vector<char>& buf)
        {
            return ReadField<uint16_t>(buf, 0);
        }

        uint64_t ParseSeq(const std::vector<char>& buf)
        {
            return ReadField<uint64_t>(buf, TYPE_LEN);
        }
        
        // 返回一个右值对象，现代编译器会优化
        std::vector<char> ParsePayload(const std::vector<char>& buf)
        {
            return std::vector<char>(buf.begin() + HEAD_LEN, buf.end());
        }

        // 将T类型的数据写如buf的offset下标
        template<class T>
        void WriteField(std::vector<char>* buf, size_t offset, const T& v) 
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if (buf->size() < offset + sizeof(v))
            {
                size_t new_size = offset + sizeof(v);
                buf->resize(new_size);
            }
            std::memcpy(buf->data() + offset, &v, sizeof(v));
        }

        void WriteType(std::vector<char>* buf)
        {
            WriteField(buf, 0, _type);
        }

        void WriteSeq(std::vector<char>* buf)
        {
            WriteField(buf, TYPE_LEN, _seq);
        }

        void WritePayload(std::vector<char>* buf)
        {
            if(buf->size() < HEAD_LEN + _payload.size())
            {
                size_t new_size = HEAD_LEN + _payload.size();
                buf->resize(new_size);
            }
            memcpy(buf->data() + HEAD_LEN, _payload.data(), _payload.size());
        }

    public:
        Message() = default;

        Message(uint16_t type, uint64_t seq, const std::vector<char>& payload)
            :_type(type)
            ,_seq(seq)
            ,_payload(payload)
        {  }

        // 序列化, 字段 --> 字节流
        bool Serializate(std::vector<char>* const out_vec)
        {
            if(out_vec == nullptr)
            {
                return false;
            }

            WriteType(out_vec);
            WriteSeq(out_vec);
            WritePayload(out_vec);

            return true;
        }

        void SetType(uint16_t type)
        {
            _type = type;
        }

        void SetSeq(uint64_t seq)
        {
            _seq = seq;
        }

        void SetPayload(const std::vector<char>& payload)
        {

            _payload = payload;
        }

        void SetPayload(std::vector<char>&& payload)
        {

            _payload = std::move(payload);
        }

        uint16_t GetType() const 
        {
            return _type;
        }

        uint64_t GetSeq() const 
        {
            return _seq;
        }

        std::vector<char> GetPayload() const 
        {
            return _payload;
        }

        // 反序列化，字节流 -> 字段
        bool Deserializate(const std::vector<char> vec)
        {
            if(vec.size() < HEAD_LEN)
            {
                return false;
            }

            _type = ParseType(vec);
            _seq = ParseSeq(vec);
            _payload = ParsePayload(vec);

            return true;
        }
    private:
        uint16_t _type;
        uint64_t _seq;
        std::vector<char> _payload;
    };

    using handler_msg_t = std::function<Message(Message)>;

    class Protocol
    {
    public:
        // 处理一个请求, 并返回一个应答
        std::vector<char> ParseProtocol(std::vector<char>& req_vec)
        {
            Message req;
            bool success = req.Deserializate(req_vec);
            if(!success)
            {
                LOG(LogLevel::ERRNO) << "反序列化处理请求失败";
                return std::vector<char>();
            }

            Message resp = _handler_msg(req);

            std::vector<char> resp_vec;
            success = resp.Serializate(&resp_vec);
            if(!success)
            {
                LOG(LogLevel::ERRNO) << "序列化处理响应失败";
                return std::vector<char>();
            }

            return resp_vec;
        }

        bool Register(handler_msg_t handler_msg)
        {
            if(handler_msg)
            {
                _handler_msg = handler_msg;
                return true;
            }
            else
            {
                return false;
            }
        }

        static std::string _version;
    private:
        handler_msg_t _handler_msg;
    };
    inline std::string Protocol::_version = "0.1.0";

}
