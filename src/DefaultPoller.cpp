#include "Poller.h"
#include <stdlib.h>
#include "EPollPoller.h"
//防止依赖倒置
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
      return nullptr;//
    }
    else
    {
      return new EPollPoller(loop);//生成epoll实例
    }
}
