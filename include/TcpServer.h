#pragma once
#include "noncopyable.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "inetAddress.h"
#include <functional>
#include <string>
#include "EventLoopThreadPool.h"
#include <memory>
#include "Callbacks.h"
#include <atomic>
#include <unordered_map>
#include "TcpConnection.h"
#include "Buffer.h"
#include "Logger.h"
//基于c++11，因此不依赖于boost
//对外的服务器变成使用的类
class TcpServer
{
public:
 using ThreadInitCallback = std::function<void(EventLoop*)>;
 
 enum Option  // 端口是否可重用
 {
   kNoReusePort,
   kReusePort
 };
 TcpServer(EventLoop *mainLoop, const InetAddress &listenAddr,const std::string &name,
           Option option = kNoReusePort);
 ~TcpServer();
void setThreadInitCallback(const ThreadInitCallback &cb)
{
  threadInitCallback_ = cb;
}

void setConnectionCallback(const ConnectionCallback &cb){
  connectionCallback_ = cb;
}

void setMessageCallback(const MessageCallback &cb){
  messageCallback_ = cb;
}
void setWriteCompleteCallback(const WriteCompleteCallback &cb){
  writeCompleteCallback_ = cb;
}

//设置底层subLoop的个数
void setThreadNum(int numThreads);

//开启服务器监听，就是开启mainLoop的acceptor的监听
void start();

private:
 void newConnection(int sockfd, const InetAddress &peerAddr);
 void removeConnection(const TcpConnectionPtr &conn);
 void renoveConnectionLoop(const TcpConnectionPtr &conn);
 using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
 

EventLoop *loop_;  // baseLoop
                   // 用户定义的loop,运行acceptor（调用socket，监听新用户连接）
const std::string ipPort_;
const std::string name_;
std::unique_ptr<Acceptor> acceptor_;  // 运行在mainLoop，任务是监听新连接事件
std::shared_ptr<EventLoopThreadPool> threadPool_;  // one loop per thread
// 用户使用的回调
ConnectionCallback connectionCallback_;        // 有新连接时的回调
MessageCallback messageCallback_;              // 有读写消息时的回调
WriteCompleteCallback writeCompleteCallback_;  // 消息发送完是的回调

ThreadInitCallback threadInitCallback_;  // loop线程初始化的回调
std::atomic_int started_;

int nextConnId_;
ConnectionMap connections_;
};