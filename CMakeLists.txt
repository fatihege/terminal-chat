cmake_minimum_required(VERSION 3.30)
project(terminal_chat)

set(CMAKE_CXX_STANDARD 20)

add_executable(terminal_chat_server main_server.cpp
        src/server/ChatServer.h
        src/server/ChatServer.cpp
        src/server/ClientHandler.h
        src/server/ClientHandler.cpp
        src/common/Commands.h
        src/common/Commands.cpp
        src/common/Constants.h
        src/common/WinsockInitializer.h
        src/common/WinsockInitializer.cpp
        src/common/Utils.cpp
        src/common/Utils.h)
target_link_libraries(terminal_chat_server ws2_32)

add_executable(terminal_chat_client main_client.cpp
        src/client/ChatClient.h
        src/client/ChatClient.cpp
        src/common/Commands.h
        src/common/Commands.cpp
        src/common/Constants.h
        src/common/WinsockInitializer.h
        src/common/WinsockInitializer.cpp
        src/common/Utils.cpp
        src/common/Utils.h)
target_link_libraries(terminal_chat_client ws2_32)
