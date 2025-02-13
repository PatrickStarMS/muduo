#include "Poller.h"
#include"Channel.h"
Poller::Poller(EventLoop* loop):ownerLoop_(loop){}
Poller::~Poller() {
    // 必须实现析构函数
}

 bool Poller::hasChannel(Channel* channel) const//当前channel是否在当前poller中
{
   auto it = channelMap_.find(channel->fd());
   return it != channelMap_.end() && it->second == channel;
 }