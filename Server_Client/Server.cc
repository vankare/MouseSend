#include <cstdint>
#include <string>
#include "Server.hpp"
using namespace NS_SERVER_MODULE;

void Usage(const std::string& process)
{
    std::cout << "Usage:\n" 
        << "\t" << process << "\tport\n";
}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        Usage(argv[0]);
        return 1;
    }
    ENABLE_CONSOLE_STRATEGY();
    LOAD_SERVER_CONFIG("../../../FFST/Config/server.toml");

    uint16_t port = static_cast<uint16_t>(std::stoul(argv[1]));
    std::unique_ptr<Server> svr(new Server(port));
    svr->Run();

    return 0;
}
