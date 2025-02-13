#include "TcpConnection.h"

#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <errno.h>
#include <memory>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
      LOG_FATAL("%s:%s:%d  mainLoop is null ,error: %d \n", __FILE__,
                __FUNCTION__, __LINE__, errno);
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                   int sockfd,const InetAddress &localAddr, const InetAddress &peerAddr)  

:loop_(CheckLoopNotNull(loop)),
name_(name),
state_(kConnecting),
reading_(true),
socket_(new Socket(sockfd)),//交给智能指针管理了，因此析构也不需要释放
channel_(new Channel(loop,sockfd)),
localAddr_(localAddr),
peerAddr_(peerAddr),
highWaterMark_(64*1024*1024)//64M
{
    //给connfd 对应的channe设置回调函数
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  LOG_INFO("TcpCOnnection::ctor{%s} at fd=%d\n", name_.c_str(), sockfd);

  socket_->setkeepAlive(true);
}

TcpConnection::~TcpConnection()                 
{
  LOG_INFO("TcpCOnnection::ctor{%s} at fd=%d  state=%d \n", name_.c_str(), channel_->fd(),static_cast<int>(state_));
}

void TcpConnection::send(const void *message, int len)              
{

}

     //关闭连接
void TcpConnection::shutdown()              
{
  if(state_==kConnected)
  {
    setState(kDisconnecting);//这里没有断开连接，是正在断开连接
    
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
  
}
//连接建立
void TcpConnection::connectEstablished()              
{
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();//向poller注册channel的epollin事件

  //连接建立执行回调
  connectionCallback_(shared_from_this());
}
void TcpConnection::connectDestroyed()              
{
  if(state_==kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());

  }
  channel_->remove();//把channel从poller删除掉
}

//下面是已连接连接的管理
void TcpConnection::handleRead(Timestamp receiveTime)              
{
    int saveErrno =0;
    //把数据读到缓冲区inputBuffer_中
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n>0)
    {
        //已建立连接的用户，有可读事件发生了。调用用户传入的回调操作omMessage
        //为什么传入当前对象的指针，因为在该函数中没还需要connection发送数据
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n==0)
    {
        //断开链接
      handleClose();
    }
    else
    {
      errno = saveErrno;
      LOG_ERROR("TcpConnection::handleRead");
      handleError();
    }
}
//如果数据没法送完，就掉这个
void TcpConnection::handleWrite()              
{
  //先判断是否可写
    if(channel_->isWriting())
    {
      int saveErrno =0;
      //将所有数据的读写都封装给了buffer
      ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
      
      if(n>0)
      {
        
        //数据发送完n个，指针复位n
        outputBuffer_.retrive(n);
        if(outputBuffer_.readableBytes()==0)
        {
          
          //发送完，现在channel不可写
          channel_->disableWriting();
          //调用回调
          if(writeCompleteCallback_)
          {
            
            //唤醒loop_对应的线程，执行回调，这里唤不唤醒无所谓，这个TcpConnection肯定是在当前线程
            loop_->queueInLoop(
                std::bind(writeCompleteCallback_, shared_from_this()));
          }
          if(state_== kDisconnecting)//
          {
            
            //如果正在断开连接，则关闭，将当前connection删除
            //如果数据发送完
            shutdownInLoop();
          }
        }
      }
      else
      {
          LOG_ERROR("TcpConnection::handleWrite");
      }
    }
    else
    {
      //不可写,会实时改变channel感兴趣的事件
      LOG_ERROR("TcpConnection fd=%d is down, no more writing \n",
                channel_->fd());
    }
}
//poller==》channel::closeCallback=>TcpConnection::handleClose
void TcpConnection::handleClose()              
{
  LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)(state_));
  //此时是已断开,设置连接状态
  setState(kDisconnected);
  //channel不感兴趣
  channel_->disableAll();

  //这不是创建新的对象，这是获取当前对象，智能指针
  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr);//TcpServer的removeConnection回调
}
void TcpConnection::handleError() { 
  int optval;
  int err = 0;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
  {
    err = errno;
  }
  else
  {
    err = optval;
  }
  LOG_ERROR("TcpConnection::handleErrno name:%s - SO_ERROR:%d \n",name_.c_str()
                ,err);
}
//发送数据，应用写的快，内核发送数据满，因此将发送数据写入缓冲区，而且设置水位回调，防止发送太快
void TcpConnection::sendInLoop(const void *message, size_t len)              
{
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if(state_==kDisconnected)
  {
    LOG_ERROR("disconnected, give up writting");
    return;
  }
  //赶赴开始注册的时候，是设置的对读事件感兴趣，没设置对写事件感兴趣
  if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0)
  {
    //channe第一册开始写数据，而且缓冲区没有带发送数据
    
    nwrote = ::write(channel_->fd(), message, len);
  if(nwrote>0)
  {
    //发送的数据可能比缓冲区大
    remaining = len - nwrote;
    if(remaining==0&&writeCompleteCallback_)
    {
      //如果刚好填满,数据全部发送完成，就不用给channel设置epollout事件了
      loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
    }
  }
  else
  {
    nwrote = 0;
    if(errno!=EWOULDBLOCK)//非阻塞返回
    {
      LOG_ERROR("TcpConnection::sendUnloop");
      if(errno==EPIPE||errno==ECONNRESET)
      {
        faultError = true;
      }
    }
  }
  }
  //当前数据没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，
  //然后给channel注册epollout事件，poller发现tcp的缓冲区哟空间，
  //会通知相应的sock channel，调用writeCallback方法，
  //也就是调用TcpConnection::handleWrite() 把发送缓冲区中的数据全部发送完成
  //epoll是监测的，如果一次发送完，就不需要使用epoll了
  if(!faultError&&remaining>0)
  {
    //目前发送缓冲区待发送数据长度
    size_t oldlen = outputBuffer_.readableBytes();
    if(oldlen+remaining>=highWaterMark_&&oldlen<highWaterMark_&&highWaterMarkCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),
                                   oldlen + remaining));
    }
    //将数据添加到缓冲区
    outputBuffer_.append((char *)message + nwrote, remaining);
    if (!channel_->isWriting()) 
    {
      //这里一定要注册channel的写事件，否则poller不会给channel通知epollout
      channel_->enableWriting();
    }
  }
}

//数据发送完，在handleWrite中也会调用这个
void TcpConnection::shutdownInLoop()              
{
  //channel没有注册写事件，说明已经发送缓冲区的数据发送完了（上面处理的，如果没法送完，就会一直发送完为止）
  
  if(!channel_->isWriting())
  {
    
    //如果数据发送完，直接掉这个
    socket_->shutdownWrite();//关闭写端，epoll就会给channal通知shutdown事件


  }
}
void TcpConnection::setState(StateE state) { state_ = state; }

void TcpConnection::send(const std::string &buf)
{
  //往outputBuffer发
  //先判断连接状态
  if(state_==kConnected)
  {
    //
    if(loop_->isInLoopThread())
    {
      sendInLoop(buf.c_str(), buf.size());

    }
    else
    {
      //不在当前线程，就先唤醒，在发送数据（回调）
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
    }
  }
}

