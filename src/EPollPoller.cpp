#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
#include<unistd.h>
#include "Channel.h"
#include <string.h>
//对应channel的成员index_,初始化为-1，对应kNew
//指的是在poller中map的状态，不是epoller中的状态
const int kNew = -1;//channel
const int kAdded = 1;
const int kDelete = 2;

EPollPoller::EPollPoller(EventLoop* loop)
:Poller(loop)
,epollFd_(::epoll_create1(EPOLL_CLOEXEC))
,events_(kInitEventListSize)
{
    if(epollFd_<0)//如果创建的EpollFd小于0，说明创建失败
    {
      LOG_FATAL("epoll crteate error: %d",errno);
      
    }
}


EPollPoller::~EPollPoller() 
{
    ::close(epollFd_);
}

//频繁调用
Timestamp EPollPoller::poll(int timeout, ChannelList* activateChannel) 
{
  //events_与channels绑定，一个channel一个event
  int numEvents=::epoll_wait(epollFd_,&*events_.begin(),static_cast<int>(events_.size()),timeout);
  int saveErrno = errno;
  Timestamp now(Timestamp::now());
  if(numEvents>0)
  {
    LOG_INFO("%d events hanppened \n", numEvents);
    fillActivateChannels(numEvents, activateChannel);
    //当vector（扩容
    if(numEvents==events_.size())
    {
      events_.resize(events_.size() * 2);
    }

  }
  else if (numEvents==0)//这一轮没有事情发生
  {
    LOG_DEBUG("%s timeout! \n", __FUNCTION__);
  }
  else//发生错误
  {
      if(saveErrno!=EINTR)//如果不是中断，就是真的发生错误了
    {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll()  err!");
    }
  }
  return now;//返回时间点
}

//channel updatechannel => eventLoop  updatechannel =>poller updatechannel =>epoll_ctl
//基于channelmap的
void EPollPoller::updateChannel(Channel* channel) 
{
//获取channel中的index_(channel的状态)，channel只调一个函数，最终poller该函数中根据channel中index的状态，对channel进行不同的操作
//通过fd操作channel（之前的映射）
const int index = channel->index();
LOG_INFO("function: %s ;fd=%d event=%d index=%d\n ", __FUNCTION__,channel->fd(), channel->events(),
         index);
//注意channel的添加删除修改,是通过chanel来调动的，channel中分成了两个函数一个是updatechannel一个是removechannel
//这里是updatechannel，不涉及删除

//往epoll里面注册
if(index==kNew||index==kDelete)
{
  if(index==kNew)
  {
    //1.添加到注册表中（map）
    int fd = channel->fd();
    channelMap_[fd] = channel;
  }
  //2.改变状态  新添加（上面已经添加到表里了）或者已删除状态（不在使用poller），但是还在poller注册表中
  channel->setIndex(kAdded);
  //3.里面调用epoll_ctl往epoll中注册
  update(EPOLL_CTL_ADD, channel);
}

//删除和修改
else
{
  int fd = channel->fd();
  //如果对任何事件都不感兴趣，说明不需要poller监听了
  if(channel->isNoneEvent())
  {
    update(EPOLL_CTL_DEL, channel);
    channel->setIndex(kDelete);
  }
  //还感兴趣，说明变了
  else
  {
    update(EPOLL_CTL_MOD, channel);
  }
 }
}


//从注册表和epoll中删除
void EPollPoller::removeChannel(Channel* channel) 
{
  
  int fd=channel->fd();
  LOG_INFO("function: %s;fd=%d \n ", __FUNCTION__,fd);
  channelMap_.erase(fd);

  int index = channel->index();

  if(index==kAdded)//
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->setIndex(kNew);
}

void EPollPoller::fillActivateChannels(int numEvents, ChannelList* activateChannels) const
{
  for (int i = 0; i < numEvents;++i)
  {
    //返回的是void*，需要进行类型转换，使用static是c++11的特性，安全转换
    Channel* channel = static_cast<Channel*> (events_[i].data.ptr);
    channel->set_revents(events_[i].events);
    activateChannels->push_back(channel);
  }
}
// 更新channel，就是调用epoll_ctl(在epoll中) add/del/mod
void EPollPoller::update(int operation, Channel* channel)
{
  epoll_event event;
  // bzero(&event,sizeof event);
  memset(&event, 0, sizeof event);
  int fd = channel->fd();
  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;

  if (::epoll_ctl(epollFd_,operation,fd,&event)<0)
  {
    if(operation==EPOLL_CTL_DEL)
    {
      LOG_ERROR("epoll_ctl del error %d \n", errno);
    
    }
    else
    {
      //如果是添加或者修改失败，在对其操作，那么就是fatal
      LOG_FATAL("epoll_add/del error:%d \n", errno);
    }
  }
}