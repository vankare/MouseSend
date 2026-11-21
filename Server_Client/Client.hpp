#include "../Transport/CalleeStrategy.hpp"
#include "../Transport/CallerStrategy.hpp"
#include "../Net/Util.hpp"
#include <memory>


namespace NS_CLIENT_MODULE
{
    using namespace NS_STRATEGY_MODULE;
    using namespace NS_UTIL_MODULE;

    enum ClientType
    {
        CalleeT,
        CallerT
    };
    class Client
    {
    public:
        // 初始化一个 callee/caller 客户端
        Client(
                const std::string& peer_id,
                ClientType type,
                const std::string& self_id = ClientConfig::_id,
                const std::string& signal_ip = ClientConfig::_server_ip,
                uint16_t signal_port = ClientConfig::_server_port,
                uint16_t callee_port = CalleeConfig::_port
            )
            :_ctype(type)
            ,_peer_id(peer_id)
        { 
            if(_ctype == CalleeT)
            {
                _callee = std::make_unique<CalleeStrategy>(self_id, signal_ip, signal_port, callee_port);
                _caller = nullptr;      // delete 一个空指针是安全的
            }
            else
            {
                _caller = std::make_unique<CallerStrategy>(self_id, signal_ip, signal_port);
                _callee = nullptr;
            }
        }

        bool Prepare(const std::string& filename = "")
        {
            bool success = true;
            if(_ctype == CalleeT)
            {
                // 连接服务端
                success &= _callee->ConnectSignal();
                // 获取对方地址
                success &= _callee->WaitForPeer(_peer_id);
                // 打洞
                success &= _callee->HolePunching();
            }
            else
            {
                if(!FileUtil::IsExists(filename))
                {
                    return false;
                }
                _caller->SendFileName() = filename;

                // 连接服务端
                success &= _caller->ConnectSignal();
                // 获取对方地址
                success &= _caller->WaitForPeer(_peer_id);
                // 打洞
                success &= _caller->HolePunching();
                // 主动发起三次握手
                success &= _caller->RequestHandshake();
            }
            return success;
        }

        bool SendData()
        {
            uint64_t file_pos = 0;
            bool ok = true;
            do
            {
                std::vector<char> send_data;
                file_pos = FileUtil::ReadFilePart(_caller->SendFileName(), file_pos, CallerConfig::_read_file_size, &send_data);
                if(file_pos)
                    ok &= _caller->StartSend(send_data, false);
                else
                    ok &= _caller->StartSend(send_data, true);
            }
            while(file_pos != 0 && ok);

            return ok;
        }

        void RecvData()
        {
            _callee->StartRecv();
        }

    private:
        std::unique_ptr<CallerStrategy> _caller;
        std::unique_ptr<CalleeStrategy> _callee;
        ClientType _ctype;
        std::string _peer_id;
    };
}
