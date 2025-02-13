#include "TcpServer.h"
#include "Logger.h"
#include <errno.h>
#include <strings.h>
#include "TcpConnection.h"
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
      LOG_FATAL("%s:%s:%d  mainLoop is null ,error: %d \n", __FILE__,
                __FUNCTION__, __LINE__, errno);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* mainLoop,const InetAddress &listenAddr,const std::string &name,Option option)
: loop_(CheckLoopNotNull(mainLoop))
,ipPort_(listenAddr.toIpPort())
,name_(name)
,acceptor_(new Acceptor(mainLoop,listenAddr,false))//acceptor调用socket监听listenAddr
,threadPool_(new EventLoopThreadPool(mainLoop,name_))
,connectionCallback_()
,messageCallback_()
,nextConnId_(1)
,started_(0)
{
    //当有新用户连接时，会执行TcpServer::newConnection回调，这里是绑定，不是调用
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2));
}
TcpServer::~TcpServer()
{
  //将所有指向connection的强智能指针关闭，自生自灭
  //关闭meigeconnection
  for(auto &item:connections_)
  {
    //本来可以直接使用item.second，但是，使用reset方法之后item.second这个指针就没有了
    //因此就没法通过该指针访问connectionDistroy方法了
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    //销毁链接
    conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
  }
}
//设置底层subLoop的个数
void TcpServer::setThreadNum(int numThreads)
{
  threadPool_->setThreadNum(numThreads);
}
//开启监听 之后loop.loop()
void TcpServer::start()
{
    if(started_++==0)//防止一个Tcpserver对象被started多次
    {
      threadPool_->start(threadInitCallback_);//启动底层线程池
      loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));//裸指针
    }
}
//有一个新的客户端的连接,Acceptor会在事件处理函数中执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
  //轮询算法，选择一个subLoop，来管理回调操作
  EventLoop *ioLoop = threadPool_->getNextLoop();
  char buf[64] = {0};
  snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;//因为只有一个线程处理链接，因此这里不需要原子变量
  std::string connName = name_ + buf;
  LOG_INFO("TcpServer::newConnection [%s] - new connection[%s] from %s",
           name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
  //  通过sockfd获取其绑定的本机ip和端口号
  sockaddr_in local;
  ::bzero(&local, sizeof local);
  socklen_t addrLen = sizeof local;
  if(::getsockname(sockfd,(sockaddr*)&local,&addrLen)<0)
  {
    LOG_ERROR("sockets::getLocalAddr");
  }
  //打包ip和port
  InetAddress localAddr(local);

  //根据连接成功的sockfd创建TcpConnection连接对象
  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName,
                                          sockfd,  // Socket  channel
                                          localAddr, peerAddr));
  //将连接存到map中
  connections_[connName] = conn;
  //下面的回调都是用户设置的==》tcpserver==》tcpconnection==》channel==》poller==》通知channel调用回调
  conn->setConnectionCallback(connectionCallback_); 
  conn->setMessageCallback(messageCallback_); 
  conn->setWriteCompleteCallback(writeCompleteCallback_); 
  //设置了如何关闭连接的回调
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
//使用subloop执行新连接
  loop_->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
  loop_->runInLoop(std::bind(&TcpServer::renoveConnectionLoop, this, conn));
}

void TcpServer::renoveConnectionLoop(const TcpConnectionPtr &conn)
{
  LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
           name_.c_str(), conn->name().c_str());

  //删除conn
  connections_.erase(conn->name());
  //获取当前的loop
  EventLoop *ioLoop = conn->getLoop();
  //绑定的不是this，而是connection
  //这是Tcpserver关闭connection，因此肯定当前线程和connection内部执行的线程不是一个
  //，因此直接掉用queueInLoop
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,  conn));
}
