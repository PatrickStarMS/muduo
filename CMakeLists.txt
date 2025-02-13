cmake_minimum_required(VERSION 3.0)

project(my_muduo)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11" )

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)
