#include "../Protocol.hpp"
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
using namespace NS_PROTOCOL_MODULE;

static std::vector<char> MakeSerializedRequest(
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

TEST(Test_Protocol, Test_Request_Parse)
{

    Request req;
    auto buf = MakeSerializedRequest(0x1, 1234567, "abcdefghi");

    req.Deserializate(buf);

    EXPECT_EQ(req.Type(), 0x1);
    EXPECT_EQ(req.Seq(), 1234567);
    EXPECT_EQ(std::string(req.Payload().begin(), req.Payload().end()), std::string("abcdefghi"));
}



int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
