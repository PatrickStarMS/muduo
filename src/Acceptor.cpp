#include "Acceptor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "Logger.h"
#include <errno.h>
#include "inetAddress.h"
static int createNonblocking()
{
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
                        0);
  if(sockfd<0)
  {
    LOG_FATAL("%s:%s:%d listen socket create err :%d \n", __FILE__,
              __FUNCTION__, __LINE__,errno);
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &address, bool reuseport)
:loop_(loop)
,acceptSocket_(createNonblocking())//创建套接字
,acceptChannel_(loop,acceptSocket_.fd())
,listenning_(false)
{
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReuseport(true);
  acceptSocket_.bindAddress(address);//绑定套接字

  //绑定回调，实现打包fd成channel并扔给subLoop
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

//客户端连接时，listenfd发生变化，调用
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    //保存客户端ip与port，返回连接的fd
    int confd = acceptSocket_.accept(&peerAddr);
    if(confd>=0)
    {
        if(newConnectionCallback_)
        {
        newConnectionCallback_(confd, peerAddr);
        }
        else
        {
        ::close(confd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept socket create err :%d \n", __FILE__,
              __FUNCTION__, __LINE__,errno);
        //如果系统可用的fd资源用完了
        if(errno==EMFILE)
        {
            //按理来说应该文件描述符上限
            LOG_ERROR("%s:%s:%d  socketfd reached limit \n", __FILE__,
              __FUNCTION__, __LINE__);
        }
    }

    
    
}
void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();//listen
    acceptChannel_.enableReading();//设置成对读事件感兴趣
}
