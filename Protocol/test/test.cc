#include "../Protocol.hpp"
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
using namespace NS_PROTOCOL_MODULE;

static std::vector<char> MakeSerializedMessage(
        uint16_t type, uint64_t seq, const std::string& data
    ) 
{
    std::vector<char> buf;
    buf.resize(sizeof(type) + sizeof(seq));
    memcpy(buf.data(), &type, sizeof(type));
    memcpy(buf.data() + sizeof(type), &seq, sizeof(seq));
    buf.insert(buf.end(), data.begin(), data.end());
    return buf;
}

static Message MakeDeserializedMessage(
    uint16_t type, uint64_t seq, const std::string& data
)
{
    std::vector<char> payload(data.begin(), data.end());
    Message msg(type, seq, payload);
    return msg;
}

namespace  NS_PROTOCOL_MODULE
{
    bool operator==(const Message& m1, const Message& m2)
    {
        auto p1 = m1.GetPayload();
        auto p2 = m2.GetPayload();
        if(p1.size() != p2.size())
        {
            return false;
        }
        return m1.GetType() == m2.GetType() && m1.GetSeq() == m2.GetSeq() && memcpy(p1.data(), p2.data(), p1.size());
    }
}

TEST(Test_Protocol, Deserializate_Serializate)
{
    Message msg1;
    std::string test_str = "abcdefghi";
    auto buf = MakeSerializedMessage(0x1, 1234567,test_str);

    msg1.Deserializate(buf);

    EXPECT_EQ(msg1.GetType(), 0x1);
    EXPECT_EQ(msg1.GetSeq(), 1234567);
    auto payload = msg1.GetPayload();
    EXPECT_EQ(std::string(payload.begin(), payload.end()), test_str);

    auto msg2 = MakeDeserializedMessage(0x1, 1234567,test_str);
    std::vector<char> serializated;
    msg2.Serializate(&serializated);
    EXPECT_EQ(msg1, msg2);
}


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
