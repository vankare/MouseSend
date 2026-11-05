#pragma once
#include <climits>
#include <set>
#include <cstdint>
#include "ServerStrategy.hpp"

namespace NS_STRATEGY_MODULE
{
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
}
