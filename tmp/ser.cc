#include "Protocol/Protocol.hpp"
#include <cstdio>
using namespace NS_PROTOCOL_MODULE;
int main(){
    Message m(SYN, 0x1122334455667788ULL, std::vector<char>({'a','b','c'}));
    std::vector<char> out;
    m.Serializate(&out);
    printf("serialized size = %zu\n", out.size());
    printf("payload size after serialize = %zu\n", m.GetPayload().size());
    return 0;
}
