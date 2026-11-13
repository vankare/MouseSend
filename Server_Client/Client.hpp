#include "../Transport/CalleeStrategy.hpp"
#include "../Transport/CallerStrategy.hpp"


namespace NS_CLIENT_MODULE
{
    using namespace NS_STRATEGY_MODULE;
    class Client
    {
        Client()
        {

        }
    private:
        CalleeStrategy _caller;
        CallerStrategy _callee;
    };
}
