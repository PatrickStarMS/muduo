#pragma once
#include <string>
#include "noncopyable.h"
#include "Timestamp.h"


//1.枚举类定义日志级别 INFO ERROR FATAL DEBUG(通过红控制)
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
} ;

//2.封装日志类。写成单例即可，继承noncopyable，就不用手写禁用拷贝和赋值构造了
class Logger:noncopyable
{
    public:
    
     static Logger& getInstance();

     void setLogLevel(int level);

     void log(std::string msg);

    private:
    int logLever_;
     Logger() {}
};


//LOG_INFO("%s,%d",arg1,arg2)
#define LOG_INFO(logmsgFormat, ...)                   \
  do {                                                \
    Logger& logger = Logger::getInstance();           \
    logger.setLogLevel(INFO);                         \
    char buffer[1024] = {0};                          \
    snprintf(buffer, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buffer);\
}while (0);


#define LOG_ERROR(logmsgFormat, ...)                   \
  do {                                                \
    Logger& logger = Logger::getInstance();           \
    logger.setLogLevel(ERROR);                         \
    char buffer[1024] = {0};                          \
    snprintf(buffer, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buffer);\
}while (0);

#define LOG_FATAL(logmsgFormat, ...)                  \
  do {                                                \
    Logger& logger = Logger::getInstance();           \
    logger.setLogLevel(FATAL);                        \
    char buffer[1024] = {0};                          \
    snprintf(buffer, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buffer);                               \
    exit(-1);\
}while (0);

#ifdef MUUODEBUG
#define LOG_DEBUG(logmsgFormat, ...)                   \
  do {                                                \
    Logger& logger = Logger::getInstance();           \
    logger.setLogLevel(DEBUG);                         \
    char buffer[1024] = {0};                          \
    snprintf(buffer, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buffer);\
}while (0);
#else
    #define LOG_DEBUG(logmsgFormat, ...)  
#endif