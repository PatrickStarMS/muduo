#include "Channel.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent=EPOLLIN |EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;

//添加通道
Channel::Channel(EventLoop* eventLoop, int fd)
    :eventLoop_(eventLoop)
    ,fd_(fd)
    ,events_(0)
    ,revents_(0)
    ,index_(-1)
    ,tied_(false)
{

}

Channel::~Channel()
{

}


//什么时候调用？
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_=obj;//弱指针观察强指针
    tied_ = true;
}

/*当改变channel所表示fd的events事件后，update负责改变在poller里面更改fd相应的事件
poller与channel是两个不相干的模块，那么怎么交互的呢，通过channel的所属的eventLoop成员变量，调用poller的相应方法，注册fd的events事件（之前的chatserver，是直接包含实现交互的，
这样两者就耦合了，通过中间件解耦）
因为这里不仅仅使用poller，还会用到eventLoop调用channel的回调函数、remove（调用eventLoop的函数删除当前的channel）等，所以就包含了eventLoop（包含poller），同时起到了poller与channel的解耦*/
void Channel::update()
{
  // add code...
  eventLoop_->updateChannel(this);
}

//通过所属的eventLoop将当前的channel从vector中删除 ，指针管理
void Channel::remove()
{
    //add code...
    eventLoop_->removeChannel(this);
}    


void Channel::handleEvent(Timestamp receiveTime)
{
    //如果绑定过，监听过当前channel
    std::shared_ptr<void> guard;
    if (tied_) 
    {
      guard = tie_.lock();//如果绑定过，就将若智能指针提成强智能指针
      if(guard)
      {
        handleEventWithGuard(receiveTime);//如果存活的话继续处理
      }
    }
    else
    {
      handleEventWithGuard(receiveTime);
    }
}

//根据接受的事件执行相应的回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  LOG_INFO("channel handleEvent revent :%d",revents_);
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (closeCallback_) {
      closeCallback_();
    }
  }

    if(revents_&EPOLLERR)
    {
        if(errorCallback_)
        {
          errorCallback_();
        }
    }

    //可读事件
    if(revents_&(EPOLLIN | EPOLLPRI))
    {
        if(readEventCallback_)
        {
          readEventCallback_(receiveTime);
        }
    }

    //可写事件
    if(revents_&(EPOLLIN | EPOLLOUT))
    {
        if(writeCallback_)
        {
          writeCallback_();
        }
    }
}