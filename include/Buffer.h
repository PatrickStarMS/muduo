#pragma once
#include <vector>
#include <memory>
#include <string>
#include<algorithm>
//网络库底层的缓冲区类型定义
class Buffer
{
    public:
    //这两个都是长度
     static const size_t kCheapPreand = 8;//开头长度
     static const size_t kInitialSize = 1024;//read和write长度

     explicit Buffer(size_t initialSize =kInitialSize):buffer_(kCheapPreand+kInitialSize),readerIndex_(+kCheapPreand),writerIndex_(+kInitialSize)
     {

     }

     size_t readableBytes() const { return writerIndex_ - readerIndex_; }
     size_t writableBytes() const { return buffer_.size() - writerIndex_; }
     size_t preandableBytes() const { return readerIndex_; }

    //返回缓冲区中可读数据的起始地址
     const char* peek() { return begin() + readerIndex_; }
    //onMessage string <-Buffer
     void retrive(size_t len)
     {
        if(len<readableBytes())//只读取了部分数据
        {
          readerIndex_ += len;
        } else  // len==readableBytes()全部读取
        {
          // 因此全部复位
          retriveAll();
        }
     }
     void retriveAll()
     {
        readerIndex_=kCheapPreand;
        writerIndex_=kCheapPreand;
     }

     //确保在写之前，先看一下，缓冲区大小够用吗，不够扩容
     void ensureWriteableBytes(size_t len)
     {
        if(writableBytes()<len)
        {
          makeSpace(len);//扩容
        }
     }

     //把onMessage函数上报的Bufffer数据，转成string类型的数据返回
     std::string retrieveAllAsString() {return  retrieveAsString(readableBytes()); }

     std::string retrieveAsString(size_t len)
     {
       std::string result(peek(), len);//截取缓冲区数据
       retrive(len);//上面将数据读出来之后，要进行缓冲区复位操作
       return result;
     }

     char* beginWrite() { return begin() + writerIndex_; }
     const char* beginWrite() const
     {
        return begin() + writerIndex_; 
     }
     void append(const char* data, size_t len) {
       ensureWriteableBytes(len);
       std::copy(data, data + len, beginWrite());
       //更新writeIndex
       writerIndex_ += len;
     }

     //从fd读取数据
     ssize_t readFd(int fd, int* saveErrno);

     ssize_t writeFd(int fd, int* saveErrno);

    private:
     // 获取vector底层的裸指针，因为socket编程用的是罗指针
      char* begin() 
     { 
        //使用的迭代器的*运算符，获取首元素的本身，然后再&取地址
        return &(*buffer_.begin()); 
    }// 获取vector底层的裸指针，因为socket编程用的是罗指针
      const char* begin() const 
     { 
        //使用的迭代器的*运算符，获取首元素的本身，然后再&取地址
        return &(*buffer_.begin()); 
    }
    void makeSpace(size_t len)
    {
        /*
         preandableBytes()返回的是可读数据的起始，
         如果有数据被读出，此时返回的就是8（kCheapPreand）+n
         ，这样就要要利用前面被读取空闲出来的数据
        */
       if(writableBytes()+preandableBytes()<len+kCheapPreand)
       {
         buffer_.resize(writerIndex_ + len);
       } else {
         // 如果前面空闲+后面空闲空间够了，就把数据往前挪，让空闲空间连续连续起来
         //copy方法
         size_t readable = readableBytes();
         std::copy(begin() + readerIndex_, begin() + writerIndex_,
                   begin() + kCheapPreand);
         readerIndex_ = kCheapPreand;
         writerIndex_ = readable + readerIndex_;
       }
    }
     std::vector<char> buffer_;//封装成vector可以自动扩容
     size_t readerIndex_;
     size_t writerIndex_;
};