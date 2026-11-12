#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONV(addr) ((struct sockaddr*)(addr))

class InetAddr
{
public:
    InetAddr() = default;
    InetAddr(const std::string& ip, uint16_t port)
        :_len(sizeof(struct sockaddr_in))
        ,_ip(ip)
        ,_port(port)
    {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        int success = inet_aton(ip.c_str(), &_addr.sin_addr);
        if(success == 0)
        {
            std::cerr << "inet_aton: bad ip" << std::endl;
        }
        _addr.sin_port = htons(port);
    }

    // 由一个字符串 "127.0.0.1:8080" 格式转 InetAddr
    InetAddr(const std::string& addr_str)
    {
        size_t pos = addr_str.find(':');
        if(pos == std::string::npos)
        {
            return;
        }

        memset(&_addr, 0, sizeof(_addr));

        std::string ip = addr_str.substr(0, pos);
        std::string port = addr_str.substr(pos + 1);
        _ip = ip;
        _port = static_cast<uint16_t>(std::stoul(port));
        InetAddr addr(_ip, _port);
        *this = addr;
    }

    InetAddr(const sockaddr_in& addr)
    {
        _addr = addr;
        _len = sizeof(struct sockaddr_in);
        _ip = inet_ntoa(addr.sin_addr);
        if(_ip.empty())
        {
            std::cerr << "inet_ntoa: bad sockaddr_in addr" << std::endl;
        }
        _port = ntohs(addr.sin_port);
    }

    const sockaddr_in* Addr() const
    {
        return &_addr;
    }

    sockaddr_in* Addr()
    {
        return &_addr;
    }

    const socklen_t Len() const 
    {
        return _len;
    }
    socklen_t Len()  
    {
        return _len;
    }

    std::string ToString() const 
    {
        return _ip + ":" + std::to_string(_port);
    }
private:
    sockaddr_in _addr;
    socklen_t _len;
    std::string _ip;
    uint16_t _port;
};
