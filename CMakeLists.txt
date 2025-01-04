cmake_minimum_required(VERSION 3.30)
project(terminal_chat)

set(CMAKE_CXX_STANDARD 20)

add_executable(terminal_chat_server main.cpp)
target_link_libraries(terminal_chat_server ws2_32)

add_executable(terminal_chat_client client.cpp)
target_link_libraries(terminal_chat_client ws2_32)
