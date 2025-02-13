#pragma once
#include "noncopyable.h"
#include <memory>
#include <atomic>
#include "inetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <string>
class Channel;
class Socket;
class EventLoop;
#include <string>

class TcpConnection :noncopyable,public std::enable_shared_from_this<TcpConnection>
                                        //得到当前对象的智能指针
{
    public:
     TcpConnection(EventLoop *loop, const std::string &name,
                   int sockfd  // tcpserver 传过来
                   ,
                   const InetAddress &localAddr, const InetAddress &peerAddr);
     ~TcpConnection();

     EventLoop *getLoop() const { return loop_; }

     std::string name() const { return name_; }

     const InetAddress &localAddr() const { return localAddr_; }
     const InetAddress &peerAddr() const { return peerAddr_; }

     bool connectioned() const { return state_ == kConnected; }

     void send(const void *message, int len);

     //关闭连接
     void shutdown();

     void setConnectionCallback(const ConnectionCallback &cb)
     {
       connectionCallback_ = cb;
     }
     void setMessageCallback(const MessageCallback &cb)
     {
       messageCallback_ = cb;
     }
     void setWriteCompleteCallback(const WriteCompleteCallback &cb)
     {
       writeCompleteCallback_ = cb;
     }
     void setCloseCallback(const CloseCallback &cb)
     {
       closeCallback_ = cb;
     }
     void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t highWaterMark)
     {
       highWaterMarkCallback_ = cb;
       highWaterMark_ = highWaterMark;
     }

    //建立连接，销毁链接
     void connectEstablished();
     void connectDestroyed();

     void handleRead(Timestamp receiveTime);
     void handleWrite();
     void handleClose();
     void handleError();

     void sendInLoop(const void *message, size_t len);
     void shutdownInLoop();
     void send(const std::string &buf);

    private:
     enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting, };
    void setState(StateE state);
     EventLoop *loop_;//这里绝对不是baseLoop，因为TcpConnection都是在subLoop中管理的
     std::atomic_int state_;
     const std::string name_;
     bool reading_;
     std::unique_ptr<Socket> socket_;
     std::unique_ptr<Channel> channel_;

    //ip和port，和Acceptor类似， Acceptor--》mainLoop 管理listenfd；TcpConnection--》subLoop  管理connfd
     const InetAddress localAddr_;
     const InetAddress peerAddr_;

     //设置回调函数，用户-》TcpServer-》TcpConnection->channel,TcpServer就相当于中专站（仅仅对于回调函数来说）
     //因此这里的和TcpServer是一样的
     //poller监听channel，调用回调
     ConnectionCallback connectionCallback_;        // 有新连接时的回调
    MessageCallback messageCallback_;              // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;  // 消息发送完是的回调
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;//当一端发送数据过快，数据量堆积到一定量后，触发，相当于水位线

    size_t highWaterMark_;//水位

    //数据缓冲区
    Buffer inputBuffer_;//
    Buffer outputBuffer_;//服务器发送数据缓冲区，send往这里发
};
