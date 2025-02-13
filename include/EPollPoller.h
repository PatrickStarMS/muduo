#pragma once
#include "Poller.h"
#include<vector>
#include<sys/epoll.h>
class EventLoop;//在基类中已经声明了，这里可以不声明

/*
**epoll的使用                           |放在
**epoll_create 创建epollfd---在构造函数 |EPollPoller中
**epoll_ctl  添加/修改/删除感兴趣的事件  |updateChannel  removeChannel
**epoll_wait  等待感兴趣时间的发生      |poll函数中

*/

class EPollPoller : public Poller {
 public:
  using EventList = std::vector<epoll_event>;
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  //重写基类poller的抽象方法
  Timestamp poll(int timeout, ChannelList* activateChannel) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

 private:
  static const int kInitEventListSize = 16;
  //填写活跃链接 
  void fillActivateChannels(int numEvents, ChannelList* activateChannels) const;
  // 更新channel，就是调用epoll_ctl(在epoll中)
  void update(int operation, Channel* channel);
  int epollFd_;
  EventList events_;
};