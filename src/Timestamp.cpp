#include "../include/Timestamp.h"
#include<time.h>


Timestamp::Timestamp():microSecondsSinceEpoch_(0)
{

}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
    
}
Timestamp Timestamp::now()//获取当前时间
{
  return Timestamp(time(NULL));
}
std::string Timestamp::toString() const
{
//把长整型转成时分秒--
char buf[128] = {0};
//转成tm的结构体
tm *tm_time=localtime(&microSecondsSinceEpoch_);
snprintf(buf, 128, "%4d/%02d/%02d %2d:%2d:%2d", tm_time->tm_year + 1900,
         tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour,
         tm_time->tm_min, tm_time->tm_sec);
return buf;
}


// #include<iostream>
// int main(int argc, const char** argv) {
//   std::cout<< Timestamp().now().toString() << std::endl;
//   return 0;
// }