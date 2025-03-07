
#include "inetAddress.h"
#include<string.h>
#include <arpa/inet.h>
InetAddress::InetAddress(uint16_t port, std::string ip )
{
    //填写sockaddr_in结构体
    //先清零
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family=AF_INET;
    addr_.sin_port = htons(port);
    //转字符串
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    
}


 //获取信息
 std::string InetAddress::toIp() const
{
 //从成员变量中读取相关信息,已经是网络字节序了，还要转成本地字节序才能正常查看
 char buf[64] = {0};
 ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
 return buf;
 }
 uint16_t InetAddress::toPort() const
 {
    return ntohs(addr_.sin_port);
 }
 std::string InetAddress::toIpPort() const
 {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);//使用strlen没有\0可以无缝添加
    uint16_t port =ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
 }


