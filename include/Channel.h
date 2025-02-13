#pragma once
#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>
#include "Logger.h"
#include "EventLoop.h"
/*channel 理解为通道，封装了socketfd和其感兴趣event，如EPOLLIN,EPOLLOUT事件
 还绑定了poller（epoll）返回的具体事件（发生的事件）
 里面有回调std::function */



class Channel {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;
  Channel(EventLoop* eventLoop, int fd);
  ~Channel();

 //调用相应的回调方法，下面的
  void handleEvent(Timestamp receiveTime);
  //设置具体的回调操作，也就是接口(下面的函数对象是私有的，写一个方法初始化，也就是指向哪个函数；handleEvent是调用回调了)
  void setReadCallback(ReadEventCallback cb) { readEventCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  int set_revents(int revent) { return revents_ = revent; }//设置已发生事件，epoll监听，调用channel的函数来设置
 

//设置该事件的类型，读or写
void enableReading()
{
    events_ |=kReadEvent;//将该时间设置成读事件
    update();//调用epoll_ctl,通知poll（epoll）将感兴趣的事件添加到epoll中
}

//去掉某一位
void disableReading() 
{ 
  events_ &= ~kReadEvent;
  update();
}

void enableWriting()
{
    events_ |=kWriteEvent;
    update();//调用epoll_ctl,通知poll（epoll）将感兴趣的事件添加到epoll中
}

void disableWriting() 
{ 
  events_ &= ~kWriteEvent;
  update();
}

void disableAll()
{
    events_=kNoneEvent;
    update();
}


//返回fd当前的事件状态
bool isNoneEvent() const { return events_ == kNoneEvent; }
bool isWriting() const { return events_ & kWriteEvent; }
bool isReading() const { return events_ & kReadEvent; }

int index() { return index_; }
void setIndex(int index) { index_ = index; }


//one loop per thread
//返回当前channel所属的eventLoop
EventLoop* ownerLoop() { return eventLoop_; }

//删除channel
void remove();

private:
 void update();//调用event_ctl定义注册事件

 //处理受保护的事件
 void handleEventWithGuard(Timestamp receiveTime);

 // 当前fd的状态,没有对任何事件感兴趣，对读/写事件感兴趣
 static const int kNoneEvent;
 static const int kReadEvent;
 static const int kWriteEvent;

 EventLoop* eventLoop_;  // 事件循环
 const int fd_;          // fd_,poller监听的对象,每个channel有自己的fd_
 int events_;  // 注册fd感兴趣的事件（已连接产生的新的套接字）
 int revents_;  // poller返回的具体发生的事件
 int index_;

 // 跨线程，生存状态的监听，防止channel被手动reamove之后，还使用channel
 std::weak_ptr<void> tie_;
 bool tied_;

 // 函数对象，绑定外部传进来的操作(就是事件的回调，根据已发生的事件revents,调用相应的回调)
 ReadEventCallback readEventCallback_;
 EventCallback writeCallback_;
 EventCallback closeCallback_;
 EventCallback errorCallback_;
};
