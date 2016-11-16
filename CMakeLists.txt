cmake_minimum_required(VERSION 3.6)
project(Project_4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/client.c
    src/server.c src/rudp_packet.c src/rudp_packet.h)

add_executable(Project_4 ${SOURCE_FILES})