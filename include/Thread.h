#pragma once
#include "noncopyable.h"
#include <functional>
#include <thread>
#include<memory>
#include <string>
#include<atomic>
class Thread:public noncopyable //记录一个线程的详细信息
{

public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc func, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();
    bool started() const { return started_; }

    pid_t tid() const { return tid_; }//和用命令行查出来的一样

    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

   private:
    void setDefaultName(); 
    bool started_;
    bool joined_;
    // std::thread
    // thread_(xxxx,xxx)会直接启动该线程，因此使用一个智能指针，决定什么时候启动·因为指针并不会创建实际变量
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    // 线程函数
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;  // 所有线程数量，因此是静态的
};