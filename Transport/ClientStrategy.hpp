#pragma once
#include <functional>
#include <sys/socket.h>
#include <vector>
#include <chrono>
#include <thread>
#include "../Message/Message.hpp"
#include "Connection.hpp"

namespace NS_STRATEGY_MODULE
{
    using namespace NS_CONNECTION_MODULE;
    using namespace NS_MESSAGE_MODULE;

    using handler_send_recv_t = std::function<void(const std::vector<char>&)>;
    using millisecond_t = std::chrono::milliseconds; 
    static const int g_max_fails = 3;
    static const char g_two_id_sep = '&';
    static const millisecond_t g_timeout(500);

    class ClientStrategy
    {
    private:

        bool SendAndRecv(const std::vector<char>& send_buf,
                Type expect_type,
                Connection& conn,
                handler_send_recv_t handler,
                int max_tries                  = g_max_fails,   // 默认 3
                millisecond_t interval         = g_timeout,     // 默认 2000ms
                millisecond_t  recv_timeout    = g_timeout)     // 默认 2000ms
        {
            conn.RecvTimeOut(recv_timeout);

            for (int i = 0; i < max_tries; ++i)
            {
                conn.Send(send_buf);

                std::vector<char> recv_vec;
                int n = conn.Recv(&recv_vec);

                if (n > 0)
                {
                    Message msg;
                    if (msg.Deserializate(recv_vec) && msg.GetType() == expect_type)
                    {
                        handler(msg.GetPayload());
                        return true;
                    }
                }
                std::this_thread::sleep_for(interval);
            }
            return false;
        }

    public:
        // 外部没有传入一个可用的套接字，创建一个客户端套接字
        ClientStrategy(const std::string& signal_id, const std::string& self_id, const std::string& signal_ip, uint16_t signal_port)
            :_self_id(self_id)
            ,_own(true)
        {
            _real_socket = new UdpSocket;
            _real_socket->BuildUdpClient();
            _signal_conn = Connection(signal_id, _real_socket, signal_ip, signal_port);
            _peer_conn._sockfd = _real_socket;
            _real_socket->SetRecvTimeout(g_timeout);
        }

        // 外部传入一个可用的套接字，不用创建
        ClientStrategy(const std::string& signal_id, const std::string& self_id, const std::string& signal_ip, uint16_t signal_port, Socket* real_socket)
            :_real_socket(real_socket)
            ,_signal_conn(signal_id, real_socket, signal_ip, signal_port)
            ,_self_id(self_id)
            ,_own(false)
        {
            _real_socket->SetRecvTimeout(g_timeout);
            _peer_conn._sockfd = _real_socket;
        }

        ~ClientStrategy()
        {
            if(_own)
            {
                delete _real_socket;
            }
        }

        // 客户端(caller, callee) 连接信令服务器
        bool ConnectSignal()
        {
            Message conn_msg(CONNECT, 0, std::vector<char>(_self_id.begin(), _self_id.end()));
            std::vector<char> conn_buf;
            conn_msg.Serializate(&conn_buf);
            bool success = SendAndRecv(conn_buf, CONNECT, _signal_conn, 
                [](const std::vector<char>&){}
            );
            if(success)
            {
                LOG(LogLevel::INFO) << "成功连上信令服务器";
            }
            else
            {
                LOG(LogLevel::ERRNO) << "无法连接信令服务器";
            }
            return success;
        }

        // 客户端获取 id 为 peer_id 的地址
        bool WaitForPeer(const std::string& peer_id)
        {
            // 构造有效载荷: self_id&peer_id 
            std::string payload = _self_id + g_two_id_sep + peer_id;
            Message two_id_msg(SIGNAL, 0, std::vector<char>(payload.begin(), payload.end()));
            std::vector<char> id_vec;
            two_id_msg.Serializate(&id_vec);

            bool success = SendAndRecv(id_vec, SIGNAL, _signal_conn,
                 [this, &peer_id](const std::vector<char>& peer_addr)
                {
                    _peer_conn._addr = InetAddr(std::string(peer_addr.begin(), peer_addr.end()));
                    _peer_conn._id = peer_id;
                },
                100,
                millisecond_t(300),
                millisecond_t(500)
            );
            if(success)
            {
                LOG(LogLevel::INFO) << "成功获取对端机器地址, addr:" << _peer_conn.AddrStr();
            }
            else
            {
                LOG(LogLevel::ERRNO) << "无法获取对端机器地址";
            }
            return success;
        }

        bool HolePunching()
        {
            Message punch_msg(PUNCH, 0, std::vector<char>());
            std::vector<char> punch_vec;
            punch_msg.Serializate(&punch_vec);

            bool success = SendAndRecv(punch_vec, PUNCH, _peer_conn,
                [](const std::vector<char>&) {  },
                10,
                millisecond_t(300),
                millisecond_t(300)
            );

            if(success)
            {
                LOG(LogLevel::INFO) << "打洞成功" << _peer_conn.AddrStr();
            }
            else
            {
                LOG(LogLevel::ERRNO) << "打洞失败";
            }
            return success;
        }


    protected:
        Socket* _real_socket;
        Connection _signal_conn;
        Connection _peer_conn;
        std::string _self_id;
        bool _own;
    };
}
