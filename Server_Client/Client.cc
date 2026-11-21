#include "Client.hpp"
#include <cstring>
#include <memory>
#include <unordered_map>
using namespace NS_CLIENT_MODULE;

void Usage(const std::string& process)
{
    std::cout << "Usage:\n\t" 
        << process << " <peer_id> <file_path> --send\n\t "
        << process << " <peer_id> --recv\n\t";
}

std::unordered_map<std::string, std::string> ParseParameter(int argc, char* argv[])
{
    if(argc < 3)
    {
        Usage(argv[0]);
        exit(1);
    }
    std::unordered_map<std::string, std::string> parameters;
    parameters["peer_id"] = argv[1];
    
    if(argc == 4 && strncmp(argv[3], "--send", strlen("--send")) == 0)
    {
        parameters["file_path"] = argv[2];
        parameters["client_type"] = argv[3];
    }
    else if(argc == 3 && strncmp(argv[2], "--recv", strlen("--recv")) == 0)
    {
        parameters["client_type"] = argv[2];
    }
    else
    {
        Usage(argv[0]);
        exit(2);
    }

    return parameters;
}


int main(int argc, char* argv[])
{
    std::unordered_map<std::string, std::string>parameters = ParseParameter(argc, argv);
    LOAD_CLIENT_CONFIG("../../../FFST/Config/client.toml");

    if(parameters["client_type"] == "--send")
    {
        std::unique_ptr<Client> client(new Client(parameters["peer_id"], CallerT));
        if(client->Prepare(parameters["file_path"]) == false)
        {
            return 1;
        }
        client->SendData();
    }
    else
    {
        std::unique_ptr<Client> client(new Client(parameters["peer_id"], CalleeT));
        if(client->Prepare() == false)
        {
            return 1;
        }
        client->RecvData();
    }

    return 0;
}
