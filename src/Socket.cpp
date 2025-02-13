#include "Socket.h"
#include <unistd.h>
#include "Logger.h"
#include <sys/types.h>       
#include <sys/socket.h>
#include "inetAddress.h"
#include <string.h>
#include <netinet/tcp.h> // For TCP_NODELAY

Socket::~Socket()
{
    ::close(sockfd_);
}


//服务器绑定的ip和端口号
void Socket::bindAddress(const InetAddress &localaddr)
{
    //绑定ip和port
    if(::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))!=0)
    {
      LOG_FATAL("bind socketfd:%d fail \n", sockfd_);
    }

}

void Socket::listen()
{
  if (::listen(sockfd_, 1024)!=0) 
  {
    LOG_FATAL("listen socketfd:%d fail \n", sockfd_);
  }
}

//连接服务的客户端的ip和port
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len=sizeof addr;
    bzero(&addr, sizeof addr);
    int confd = ::accept4(sockfd_,(sockaddr *)&addr, &len,SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(confd>=0)
    {
      peeraddr->setSockAddr(addr);
    }
    return confd;
}

void Socket::shutdownWrite()
{
    //只关闭远端
    if(::shutdown(sockfd_,SHUT_WR)<0)
    {
      LOG_ERROR("shutdown sockfd_:  %d error", sockfd_);
    }
}

//协议级别
void Socket::setTcpNoDelay(bool on)
{
    int optval= on ? 1 :0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

//socket级别，因此该第二个参数level
void Socket::setReuseAddr(bool on)
{
    int optval= on ? 1 :0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReuseport(bool on)
{
    int optval= on ? 1 :0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setkeepAlive(bool on)
{
    int optval= on ? 1 :0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}


