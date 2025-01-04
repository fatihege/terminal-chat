cmake_minimum_required(VERSION 3.30)
project(terminal_chat)

set(CMAKE_CXX_STANDARD 20)

add_executable(terminal_chat main.cpp)

target_link_libraries(terminal_chat ws2_32)
