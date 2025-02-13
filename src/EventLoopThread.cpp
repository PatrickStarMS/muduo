#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
    const std::string &name ):loop_(nullptr),exiting_(false),thread_(std::bind(&EventLoopThread::threadFunc,this),name),mutex_(),cond_(),callback_(cb)
    
    {

    }

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr)
    {
      loop_->quit();
      thread_.join();//等待底层子线程结束
    }
}

//为什么在线程函数中已经绑定了loop_,还要再返回？因为，这是两个线程，不知道另外一个线程什么时候绑定好，
//因此在这里等到绑定好，返回
EventLoop* EventLoopThread::startLoop()
{
    thread_.start();

    //等待一个loop绑定一个thread完成后，通知这里，继续执行，获取绑定的loop_(新线程中执行的)
    EventLoop *loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);

        while(loop_==nullptr)
        {
          cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

//是在单独的新线程中运行的，因为传给的是新线程（线程方法
void EventLoopThread::threadFunc()
{
    EventLoop loop;//在线程内部创建一个loop，一一对应

    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;//进行绑定
        cond_.notify_one();
    }

    loop.loop();//线程中执行eventLoop的loop函数，开始监听
    //上面是一个循环，除非，quit_=false,也就是不监听了，才执行下面，
    //不监听了，loop_也要置空，以便下一次正常使用
    
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
