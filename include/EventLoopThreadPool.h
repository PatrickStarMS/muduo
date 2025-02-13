#pragma once

#include "noncopyable.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool :noncopyable
{
public:
     using ThreadInitCallback = std::function<void(EventLoop*)>;
     EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
     ~EventLoopThreadPool();

     void setThreadNum(int numThreads) { numThreads_ = numThreads; }

     void start(const ThreadInitCallback &cb = ThreadInitCallback());

     //如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
     EventLoop *getNextLoop();
     std::vector<EventLoop *> getAllLoops();

     bool started() const { return started_; }

     const std::string name() { return name_; }

private:
     EventLoop *baseLoop_;  // 最少一个loop
     std::string name_;
     bool started_;
     int numThreads_;
     int next_;
     std::vector<std::unique_ptr<EventLoopThread>> threads_;
     std::vector<EventLoop *> loops_;//这里没有使用智能指针，生命周期是外部管理 loop.loop()一直循环，不会跳出{}，因此局部变量也就不会释放
};