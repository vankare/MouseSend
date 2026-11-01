#include <chrono>
#include <gtest/gtest.h>
#include "../Util.hpp"
using namespace NS_UTIL_MODULE;

TEST(TestUtil, TestTimeUtil)
{
    int count = 5432100;
    std::chrono::microseconds time_interval(count);
    struct timeval tv = TimeUtil::ToTimeval(time_interval);

    EXPECT_EQ(tv.tv_sec, count / 1000000);
    EXPECT_EQ(tv.tv_usec, count % 1000000);
}


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
