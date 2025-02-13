#include "EventLoop.h"
#include<unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <Logger.h>
#include "Poller.h"
#include "Channel.h"
#include <memory>
//防止一个线程创建多个EventLoop，和前面的一个线程一个tid一样
//指针为空说明接下来第一次创建，不为空就不创建了 
__thread EventLoop *t_loopInThisThread = nullptr;

//定义默认poller，io复用接口的超时函数
const int kPollTimeMs = 10000;

// 创建wakeupfd,用来notify唤醒subReactor处理新的channel;轮询唤醒方式
int createEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);  // io都是返回正整数
  if (evtfd < 0) {
    LOG_FATAL("eventfd error: %d \n", errno);
  }
  return evtfd;
}
//成员变量（回调）初始化
EventLoop::EventLoop()
:looping_(false)
,quit_(false)
,callingPendingFunctions_(false) 
,threadId_(CurrentThread::tid())
,poller_(Poller::newDefaultPoller(this))
,wakeupFd_(createEventfd())
,wakeupChannel_(new Channel(this,wakeupFd_))//之所以用new，是因为EventLoop使用智能指针管理
,currentActivateChannel_(nullptr)
{
  LOG_DEBUG("EventLoop create %p in thread %d \n", this, threadId_);
  if(t_loopInThisThread)
  {
    LOG_FATAL("Another EventLoop %p exist in this thread %d \n",
              t_loopInThisThread, threadId_);

  }
  else{
    t_loopInThisThread = this;
  }

  //设置wakeupfd的事件类型以及发生事件后的回调操作
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  //每一个eventLoop都监听wakeupchannel的EPOLLIN读事件,当wakeup往wakeupfd写一个数据，就会被唤醒
  wakeupChannel_->enableReading();
}

//用来唤醒loop所在的线程，向wakeupf写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒（通过eventfd唤醒），然后执行loop 》 poller的poll
void EventLoop::wakeup()
{
    uint64_t one=1;//写啥无所谓，只要写就行
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n!=sizeof one)
    {
      LOG_ERROR("EventLoo::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

EventLoop::~EventLoop()
{
    //对所有时间都不感兴趣
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();//把channel本身从poller中删除
    ::close(wakeupFd_);
}

//清除计数器，并确保被唤醒
void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n!=sizeof one)
    {
      LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

//调用底层poller,事件循环
void EventLoop::loop()
{
    looping_=true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);
    while(!quit_)
    {
      activateChannels_.clear();
      //poll函数里面调用epoll_wait等待事件的发生，并通过fillActivateChannels函数将已发生的事件放在activateChannels_中，返回发生的时间
      pollReturnTime_ = poller_->poll(kPollTimeMs, &activateChannels_);

     //遍历每个已发生的channel，处理事件，调用各自的回调
     for(Channel* channel :activateChannels_)
     {
       channel->handleEvent(pollReturnTime_);
     }
     //执行当前EventLoop事件循环需要处理的回调操作
    /*
    ** IO线程 mainLoop 做的是accept的操作接受新用户的连接 ，返回一个fd，将其打包成channel
    已连接用户的channel分发给subLoop 
    mianLoop实现注册一个回调cb（需要subLoop来执行） wakeup 
    */
     doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping . \n", this);
    looping_ = false;
}

//1.loop在自己的线程中调用quit()将quit_设成false，结束loop()
//2.其他线程中调用当前的quit(),不知道当前线程什么情况，需要先唤醒
void EventLoop::quit()
{
    quit_=true;

    if(!isInLoopThread())
    {
      wakeup();
    }

}


void EventLoop::runInLoop(Functor cb)//在当前Loop中执行
{
    if(isInLoopThread())
    {
      
      cb();
    }
    else
    {
      queueInLoop(std::move(cb));
    }
}


void EventLoop::queueInLoop(Functor cb)//放到队列中，唤醒并转到loop对应的线程中执行cb
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctions_.emplace_back(cb);
    }

    //唤醒相应的，执行上面回调操作的loop的线程
    ///1.别的线程中；2.当前线程 的lopp中正在执行回调，现在又往doPendingFunctors写回调（不是往wakefd写，因此没有操作），但是下一个loop有阻塞在poll，导致新的回调没有被执行
    //，现在使用wakeup进行唤醒（往wakeupfd写东西（快瞬时完成），使其变成可读），此时阻塞在epoll_wait的loop就会检测到事件的变化，从而被唤醒
    
    if(!isInLoopThread()||callingPendingFunctions_)
    {
      wakeup();//唤醒loop所在线程
      

    }
}

void EventLoop::updateChannel(Channel *channel)
{
  poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
  poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}



void EventLoop::doPendingFunctors()
{
    //这里的pendingFunctions_加锁了，因此当取函数的时候，往里加函数就会阻塞，导致服务器时延
    //这里直接一次将所有的待处理的回调取出，不用频繁的获取锁,这样之后的读与取是在两个不同的vector中，没有干扰
    std::vector<Functor> functors;
    callingPendingFunctions_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctions_);
    }

    //取回调，执行
    for(const auto &functor:functors)
    {
      functor();
    }
    callingPendingFunctions_ = false;
}