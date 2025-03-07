#!/bin/bash

set -e

#如果没有build目录，创建该目录
if [ ! -d $(pwd)/build ]; then
    mkdir $(pwd)/build
fi

rm -rf $(pwd)/build/*

cd $(pwd)/build &&
    cmake .. &&
    make

#回到根目录
cd ..

#把头文件拷贝到 /usr/include/mymuduo  so拷贝到 /usr/lib
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in $(find ./include -name "*.h")
do
    cp $header /usr/include/mymuduo
done

cp $(pwd)/lib/libmymuduo.so /usr/lib

#刷新动态库缓存
ldconfig