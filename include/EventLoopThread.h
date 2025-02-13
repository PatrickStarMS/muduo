#pragma once
#include "noncopyable.h"
#include <functional>
#include "Thread.h"
#include <memory>
#include <condition_variable>
#include <string>
class EventLoop;

class EventLoopThread :public noncopyable //实现one loop per thread
{
    public:
    //初始化函数
     using ThreadInitCallback = std::function<void(EventLoop*)>;
     EventLoopThread(const ThreadInitCallback &cb=ThreadInitCallback(),const std::string &name = std::string());

     ~EventLoopThread();
     EventLoop *startLoop();

    private:
     void threadFunc();
     EventLoop *loop_;
     bool exiting_;
     //这里是Thread，不是std::thread,因此可以使用
     Thread thread_;
     std::mutex mutex_;
     std::condition_variable cond_;
     ThreadInitCallback callback_;
};