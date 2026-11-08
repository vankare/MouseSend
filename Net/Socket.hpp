#pragma once
#include "InetAddr.hpp"
#include "Util.hpp"
#include <asm-generic/socket.h>
#include <cstring>
#include <utility>
#include <vector>
#include <unistd.h>
#include <chrono>
#include <sys/socket.h>
#define BUF_SIZE 10240

namespace NS_SOCKET_MODULE
{
    class Socket
    {
    protected:
        virtual bool CreateSocket() = 0;
        virtual bool Bind(const InetAddr& addr) = 0;
    public:
        virtual int Send(std::vector<char>in_data, const InetAddr& addr) = 0;
        virtual int Recv(std::vector<char>* out_data, InetAddr* out) = 0;

        bool BuildUdpServer(uint16_t port)
        {
            bool success = CreateSocket();
            success &= Bind({"0.0.0.0", port});

            return success;
        }

        bool BuildUdpClient()
        {
            bool success = CreateSocket();
            return success;
        }

        virtual ~Socket()
        {
            close(_sockfd);
        }

        bool SetRecvTimeout(std::chrono::milliseconds time_interval)
        {
            struct timeval tv;
            tv = NS_UTIL_MODULE::TimeUtil::ToTimeval(time_interval);

            int n = setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if(n < 0)
            {
                return false;
            }
            return true;
        }
    protected:
        int _sockfd;
    };

    class UdpSocket : public Socket
    {
    public:
        bool CreateSocket() override
        {
            _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if(_sockfd < 0)
            {
                return false;
            }
            return true;
        }

        bool Bind(const InetAddr& addr) override
        {
            int n = bind(_sockfd, (struct sockaddr*)addr.Addr(), addr.Len());
            if(n != 0)
            {
                return false;
            }
            return true;
        }

        int Send(std::vector<char> in_data, const InetAddr& addr) override
        {
            if(in_data.empty())
            {
                std::cerr << "Send: in_data is empty" << std::endl;
            }
            int n = sendto(_sockfd, in_data.data(), in_data.size(), 0, (struct sockaddr*)addr.Addr(), addr.Len());
            return n;
        }

        int Recv(std::vector<char>* out_data, InetAddr* peer) override
        {
            if(out_data == nullptr)
            {
                std::cerr << "recv: out_data is nullptr" << std::endl;
                return -1;
            }

            std::vector<char> buf(BUF_SIZE);
            sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            int n = recvfrom(_sockfd, buf.data(), BUF_SIZE, 0, (sockaddr*)&addr, &addr_len);
            buf.erase(buf.begin() + n, buf.end());
            if(n > 0)
            {
                out_data->resize(n);
                *out_data = std::move(buf);
            }
            else
            {
                std::cerr << "WARNING, 错过一个数据包" << std::endl;
            }
            if(peer != nullptr)
            {
                *peer = InetAddr(addr);
            }
            return n;
        }
    };
}
