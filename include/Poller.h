#pragma once
#include"noncopyable.h"

#include <vector>
#include <unordered_map>
#include "Timestamp.h"
class Channel;//使用指针可以前向声明
class EventLoop;
// 事件分发器中的io复用模块，负责事件监听epoll_wait
class Poller:noncopyable
{
public:
 using ChannelList = std::vector<Channel*>;//对某些事件感兴趣的channel
 Poller(EventLoop* loop);

 virtual ~Poller();
//给所有的io复用保留统一的接口
 virtual Timestamp poll(int timeout, ChannelList* activateChannels) = 0;
 virtual void updateChannel(Channel* channel) = 0;//channel的update调用，用于更新
 virtual void removeChannel(Channel* channel) = 0;//删除channel
 bool hasChannel(Channel* channel) const;//当前channel是否在当前poller中，通过channel的fd找到对应的channel（channelmap）

 //获取默认的io多路复用，poll还是epoll，因为是要统一接口，不能写死
 static Poller* newDefaultPoller(EventLoop* loop);

protected:

//key:sockfd  value sockfd所属的channel
 using ChannelMap = std::unordered_map<int, Channel*> ;
 ChannelMap channelMap_;

private:
 // poller所属eventLoop
 EventLoop* ownerLoop_;

};