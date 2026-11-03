#include "../Socket.hpp"
#include <memory>
#include <cstring>
using namespace NS_SOCKET_MODULE;

void test_client()
{
    std::shared_ptr<Socket> sockfd(new UdpSocket);
    sockfd->BuildUdpClient();

    InetAddr addr("127.0.0.1", 4000);
    while(true)
    {
        std::vector<char> data;
        std::string message;
        std::cin >> message;
        data.resize(message.size());
        memcpy(data.data(), message.data(), message.size());
        sockfd->Send(data, addr);
        data.push_back(0);
        printf("%s\n", (const char*)data.data());
        sockfd->Recv(&data, nullptr);
    }
}

int main()
{
    test_client();
    return 0;
}
