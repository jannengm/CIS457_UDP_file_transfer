cmake_minimum_required(VERSION 3.6)
project(Project_4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -pthread")

set(SOURCE_FILES
    src/client.c src/rudp_packet.c src/rudp_packet.h src/window.c src/window.h src/cache_list.c src/cache_list.h)
find_package (Threads)
add_executable(Project_4 ${SOURCE_FILES})
target_link_libraries (Project_4 ${CMAKE_THREAD_LIBS_INIT})