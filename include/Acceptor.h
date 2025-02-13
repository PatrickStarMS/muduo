#pragma once
#include "Socket.h"
#include "noncopyable.h"
#include "Channel.h"
#include <functional>
class EventLoop;
class InetAddress;

class Acceptor:noncopyable
 
{
    public:
     using NewConnectionCallback =
         std::function<void(int sockfd, const InetAddress &)>;
     Acceptor(EventLoop *loop, const InetAddress &address, bool reuseport);
     ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
      newConnectionCallback_ = cb;
    }
    void listen();
    bool listenning() const { return listenning_; }

   private:
    void handleRead();
    EventLoop *loop_;  // Acceptor用的就是用户定义的那个baseLoop，也就是mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    bool listenning_;
    NewConnectionCallback newConnectionCallback_;
};