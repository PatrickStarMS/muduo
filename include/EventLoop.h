#pragma once
#include "noncopyable.h"
#include <functional>
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>
#include "CurrentThread.h"
#include <mutex>
class Channel;
class Poller;

// 事件循环类，主要包含了两个大模块Channel Poller（epoll的抽象）
// 一个线程，一个事件循环（EventLoop），一个poller，一堆channel
class EventLoop
{
    public:
    //类型重命名
     using Functor = std::function<void()>;//定义回调函数类型

     EventLoop();
     ~EventLoop();

     void loop();//开启时间循环
     void quit();//退出事件循环

     Timestamp pollReturnTime() { return pollReturnTime_; }

     void runInLoop(Functor cb);//在当前Loop中执行
     void queueInLoop(Functor cb);//放到队列中，唤醒并转到loop对应的线程中执行cb

     void wakeup();//唤醒loop对应的线程

    //调用poller的对应方法
     void updateChannel(Channel *channel);
     void removeChannel(Channel *channel);
     bool hasChannel(Channel *channel);

    //判断EventLoop对象是否在自己的线程中
     bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    private:
     using ChannelList = std::vector<Channel*>;
     std::atomic_bool looping_;//底层CAS
     std::atomic_bool quit_;//标志退出loop循环

     void handleRead();
     void doPendingFunctors();//执行回调

     const pid_t threadId_;//当前loop所在线程id
     Timestamp pollReturnTime_;//poller返回发生事件的channels的时间点

     std::unique_ptr<Poller> poller_;

     int wakeupFd_;//主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
     std::unique_ptr<Channel> wakeupChannel_;//在poller里面直接操作的都是channel，不会操作fd，因此要将wakefdz_封装在这里面

     ChannelList activateChannels_;
     Channel *currentActivateChannel_;//断言操作

      std::atomic_bool callingPendingFunctions_;//标识当前loop是否有需要执行的回调操作
      std::vector<Functor> pendingFunctions_;//存放上述回调操作
      std::mutex mutex_;  // 保护存放回调操作的vector，线程安全
};