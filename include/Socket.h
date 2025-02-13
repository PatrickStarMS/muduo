#pragma once
#include "noncopyable.h"

class InetAddress;
class Socket :noncopyable
{
public:
     explicit Socket(int sockfd):sockfd_(sockfd)
     {

     }
     ~Socket();
     int fd() const { return sockfd_; }

     void bindAddress(const InetAddress &localaddr);

     void listen();
     //返回客户端连接服务器的fd，以及将客户端的ip和port打包传出去
     int accept(InetAddress *peeraddr);

     void shutdownWrite();

     void setTcpNoDelay(bool on);
     void setReuseAddr(bool on);
     void setReuseport(bool on);
     void setkeepAlive(bool on);

    private:
     const int sockfd_;
};