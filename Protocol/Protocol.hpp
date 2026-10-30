#pragma once
#include "ConnectionManager.hpp"
#include "../Comm/InetAddr.hpp"
#include "../Comm/Logger.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <functional>

namespace NS_PROTOCOL_MODULE
{
#define TYPE_LEN 2
#define SEQ_LEN 8
#define MAX_TOTAL_LEN 10240
#define HEAD_LEN (TYPE_LEN + SEQ_LEN)
#define MAX_DATA_LEN (MAX_TOTAL_LEN - HEAD_LEN)

    using namespace NS_CONNECTIOM_MANAGER_MODULE;
    using namespace NS_LOGGER_MUDULE;

    class Request
    {
    private:
        // 提取buf下标为offset的内存变量
        template <class T>
        T ReadField(const std::vector<char>& buf, size_t offset) 
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if (offset + sizeof(T) > buf.size())
            {
                LOG(LogLevel::ERRNO) << "read_field: buffer too small";
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
    public:
        // 反序列化一个请求
        bool Deserializate(const std::vector<char>& req_vec)
        {
            if(req_vec.size() < TYPE_LEN + SEQ_LEN)
            {
                return false;
            }

            _type = ParseType(req_vec);
            _seq  = ParseSeq(req_vec);
            _payload = ParsePayload(req_vec);

            return true;
        }

        uint16_t Type() const 
        {
            return _type;
        }

        uint64_t Seq() const 
        {
            return _seq;
        }

        std::vector<char> Payload() const
        {
            return _payload;
        }

        std::vector<char>& Payload()
        {
            return _payload;
        }
    private:
        uint16_t _type;
        uint64_t _seq;
        std::vector<char> _payload;
    };

    class Response
    {
    private:
        // 将T类型的数据写如buf的offset下标
        template<class T>
        void WriteField(std::vector<char>* buf, size_t offset, const T& v) 
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if (buf->size() < offset + sizeof(v))
            {
                size_t new_size = offset + sizeof(v);
                _payload.resize(new_size);
            }
            std::memcpy(_payload.data() + offset, &v, sizeof(v));
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
                _payload.resize(new_size);
            }
            memcpy(buf->data() + HEAD_LEN, _payload.data(), _payload.size());
        }
    public:
        Response(uint16_t type, uint16_t seq, const std::vector<char>&_payload)
            :_type(type)
            ,_seq(seq)
            ,_payload(_payload)
        {  }

        Response(uint16_t type, uint64_t seq, std::vector<char>&&_payload)
            :_type(type)
            ,_seq(seq)
            ,_payload(std::move(_payload))
        {  }

        Response() = default;

        void SetType(uint16_t type)
        {
            _type = type;
        }

        void SetSeq(uint64_t seq)
        {
            _seq = seq;
        }

        void SetPayLoad(const std::vector<char>& payload)
        {

            _payload = payload;
        }

        void SetPayLoad(std::vector<char>&& payload)
        {

            _payload = std::move(payload);
        }

        // 序列化一个响应
        bool Serializate(std::vector<char>* out_vec)
        {
            if(out_vec == nullptr)
            {
                return false;
            }

            out_vec->clear();
            WriteType(out_vec);
            WriteSeq(out_vec);

            return true;
        }
    private:
        uint16_t _type;
        uint64_t _seq;
        std::vector<char> _payload;
    };

    using handler_t = std::function<Response(Request, InetAddr*)>;

    class FFSTProtocol
    {
    public:
        // 处理一个请求, 并返回一个应答
        std::vector<char> ParseProtocol(std::vector<char>& req_vec, InetAddr* out_addr)
        {
            Request req;
            bool success = req.Deserializate(req_vec);
            if(!success)
            {
                LOG(LogLevel::ERRNO) << "反序列化处理请求失败";
                return std::vector<char>();
            }

            Response resp = _handler(req, out_addr);

            std::vector<char> resp_vec;
            success = resp.Serializate(&resp_vec);
            if(!success)
            {
                LOG(LogLevel::ERRNO) << "序列化处理响应失败";
                return std::vector<char>();
            }

            return resp_vec;
        }

        static std::string _version;
    private:
        ConnectionManager _connections;
        handler_t _handler;
    };
    inline std::string FFSTProtocol::_version = "0.1.0";

}
