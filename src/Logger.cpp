#include "Logger.h"
#include <time.h>
#include<iostream>

Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    logLever_=level;
}

void Logger::log(std::string msg)
{
    //这里打印到标准控制台（之前的是打印到文件中）
    //多分类--》switch
    switch(logLever_)
    {  
    case  INFO  :
      std::cout << "[INFO]";
      break;
    case  ERROR  :
      std::cout << "[ERROR]";
      break;
    case  FATAL  :
      std::cout << "[FATAL]";
      break;
    case  DEBUG  :
      std::cout << "[DEBUG]";
      break;
    default:
        break;
    }
    //打印时间和msg
   std:: cout << Timestamp().now().toString() << " : " << msg << std::endl;
}
