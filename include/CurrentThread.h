#pragma once
#include<unistd.h>
#include<syscall.h>
namespace CurrentThread
{
extern __thread int t_cachedTid;

void cacheTid();

inline int tid()
{
    //如果没有存储，说明没有获取
    if(__builtin_expect(t_cachedTid==0,0))
    {
      cacheTid();
    }
    return t_cachedTid;
};
}