#include <mymuduo/TcpServer.h>
#include <string>
class Myserver
{
public:
Myserver(EventLoop *loop, const InetAddress &addr,const std::string &name):
server_(loop,addr,name),loop_(loop)
{
  server_.setConnectionCallback(
      std::bind(&Myserver::onConnect, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&Myserver::onMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  server_.setThreadNum(3);
}
void start() { server_.start(); }

private:
void onConnect(const TcpConnectionPtr &conn)
{
    if(conn->connectioned())
    {
      LOG_INFO("connected success");
      
    }
    else
    {
        LOG_INFO("connected failed");
    }
}
void onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time)
{
  std::string msg=buf->retrieveAllAsString();
  conn->send(msg);
  conn->shutdown();//关闭写端  EPOLLHUB=>closeCallback_
}
 EventLoop *loop_;
 TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);

    Myserver server(&loop, addr, "mymuduo_server");
    server.start();//listen loopthread listenfd=>accepchannel-》mainLoop
    loop.loop();//启动mainLoop的底层Poller
    return 0;
}