#include "../Socket.hpp"
#include <memory>
using namespace NS_SOCKET_MODULE;

void test_server()
{
    std::shared_ptr<Socket> sockfd(new UdpSocket);
    sockfd->BuildUdpServer(4000);
    
    int cnt = 10;
    while(true)
    {
        std::vector<char> data(1024, 0);
        InetAddr peer;
        sockfd->Recv(&data, &peer);
        printf("%s\n", (const char*)data.data());
        sockfd->Send(data, peer);
    }
}

int main()
{
    test_server();
    return 0;
}
