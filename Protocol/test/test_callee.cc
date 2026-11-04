#include "../TransportStrategy.hpp"
#include <fstream>
#include <gtest/gtest.h>
using namespace NS_PROTOCOL_MODULE;

TEST(Strategy, CalleeStrategy)
{
    CalleeStrategy callee(
        [](const std::vector<char>& data) ->std::vector<char>
        {
            std::ofstream ofs("recv.txt");
            if(!ofs.is_open())
            {
                std::cerr << "ifstream err" << std::endl;
            }

            ofs.write(data.data(), data.size());

            return std::vector<char>();
        },
        3000
    );
    callee.StartRecv();
}


int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
