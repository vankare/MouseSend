#include "../Transport/SignalStrategy.hpp"

namespace NS_SERVER_MODULE
{
    using namespace NS_STRATEGY_MODULE;
    class Server
    {
    public:
        Server(uint16_t port)
            :_ss(port)
        {  }

        void Run()
        {
            _ss.StartRecv();
        }
    private:
        SignalStrategy _ss;
    };
}
