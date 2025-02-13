#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
//从fd上读取数据   Poller工作模式在LT模式
//buffer缓冲区是有大小的，但是从fd上读取数据的时候，却不知道tcp数据最终的大小
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536]={0};//栈上内存
    struct iovec vec[2];
    const size_t writable = writableBytes();//这是Buffer底层缓冲区剩余的可写空间大小

    //第一块缓冲区
    vec[0].iov_base = begin() + writerIndex_;//找到可写数据区的起始地址
    vec[0].iov_len = writable;
    //第二块缓冲区，先用第一一块，第一块不够用再用第二块，然后将第二块再写入到buffer中
    //这样做的好处是，栈起了一个缓冲作用，可以一次性写入固定大小的数据（知道怎么扩容）
    //如果是流式数据直接放到缓冲区，不知道怎么扩容
    vec[1].iov_base = extrabuf;
    vec->iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf ? 2 : 1);
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if(n<0)
    {
      *saveErrno = errno;
    }
    else if(n<writable)
    {
        //buffer本身空间就够了，更新可写缓冲区起始地址
        writerIndex_ += n;
    }

    else//extra里面也写入了数据
    {
        //更新可写缓冲区起始地址
        writerIndex_ = buffer_.size();
        // 将extrabuf中的数据放入到缓冲区
        append(extrabuf, n - writable);

    }
    return n;
}

 ssize_t Buffer::writeFd(int fd, int* saveErrno)
 {
    
    ssize_t n=::write(fd,peek(),readableBytes());
    if(n<0)//出错
    {
      *saveErrno = errno;
    }
    //等于0，什么也没写，外面判断了=0的情况
    return n;
 }