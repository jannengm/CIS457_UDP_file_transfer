cmake_minimum_required(VERSION 3.6)
project(Project_4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/client.c
    src/server.c)

add_executable(Project_4 ${SOURCE_FILES})