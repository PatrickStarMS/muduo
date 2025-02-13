#pragma once

/*当调用派生类的拷贝构造与赋值构造时会先调用基类的，
基类的的这两个delete，那么派生类的默认delete*/

class noncopyable
{
    public:
     noncopyable(const noncopyable&) = delete;
     noncopyable& operator=(const noncopyable&) = delete;
     protected:
      noncopyable() = default;
      ~noncopyable() = default;
};