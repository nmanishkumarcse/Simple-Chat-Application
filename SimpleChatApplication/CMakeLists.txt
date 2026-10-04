cmake_minimum_required(VERSION 3.16)
project(SimpleChatApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(chat_server
    server/main.cpp
    server/ChatServer.cpp
    server/ClientHandler.cpp
    server/Authentication.cpp
    server/MessageManager.cpp
    server/RoomManager.cpp
    server/Database.cpp)
target_include_directories(chat_server PRIVATE include server)
target_link_libraries(chat_server PRIVATE SQLite::SQLite3 Threads::Threads)

add_executable(chat_client
    client/main.cpp
    client/ChatClient.cpp)
target_include_directories(chat_client PRIVATE include client)
target_link_libraries(chat_client PRIVATE Threads::Threads)
