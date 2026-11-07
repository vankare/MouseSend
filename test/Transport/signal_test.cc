#include "../../Transport/SignalStrategy.hpp"
#include <gtest/gtest.h>
using namespace NS_STRATEGY_MODULE;

TEST(Strategy, SignalStrategy)
{
   SignalStrategy ss(8888);
   ss.StartRecv();
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
