#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

//静态成员变量类外单独初始化
std::atomic_int Thread::numCreated_ (0);//使用构造函数初始化

Thread::Thread(ThreadFunc func, const std::string &name)
:started_(false),joined_(false),tid_(0),func_(std::move(func)),name_(name)
{
  setDefaultName();
}
Thread::~Thread()
{
    //设置成分离线程，内核自己回收
    if(started_&&!joined_)
    {
      thread_->detach();
    }
}

void Thread::start()
{
    started_=true;
    sem_t sem;
    sem_init(&sem, false, 0);//第二个参数，设置是否进程间共享
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
      // 获取线程tid
      tid_ = CurrentThread::tid();
      sem_post(&sem);
      func_();
      
    }));

    //子线程在执行上面的，只有等上面的线程执行完，才能获取tid_，因此这里使用信号量
    sem_wait(&sem);
}

void Thread::join()
{
    joined_=true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
      char buf[32];
      snprintf(buf, sizeof buf, "Thread %d", num);
      name_ = buf;
    }
}