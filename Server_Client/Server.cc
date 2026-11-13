#include "../Transport/SignalStrategy.hpp"
#include <cstdint>
#include <string>

void Usage()
{

}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        Usage();
        return 1;
    }

    ENABLE_FILE_STRATEGY();
    uint16_t port = static_cast<uint16_t>(std::stoul(argv[1]));
    NS_STRATEGY_MODULE::SignalStrategy signal(port);


    return 0;
}
