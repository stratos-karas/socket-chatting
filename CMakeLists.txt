cmake_minimum_required(VERSION 3.26)
project(SocketChatting)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(client src/client.cpp)
add_executable(server src/server.cpp src/packaged_msg.cpp)
add_executable(client-gui src/client-gui.cpp src/packaged_msg.cpp)
include_directories(include/)

add_subdirectory(external/FTXUI)

target_link_libraries(client dom screen component)
target_link_libraries(server dom screen component)
target_link_libraries(client-gui dom screen component)
