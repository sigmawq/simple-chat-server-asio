cmake_minimum_required(VERSION 3.17)
project(simple-chat-server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread")
find_package(Boost REQUIRED)

add_executable(scs-server server/compile-server.cpp common/scs-utility.h common/message.h client/client.h server/server.h)
add_executable(scs-client client/compile-client.cpp common/scs-utility.h common/message.h client/client.h server/server.h common/user_actions.h common/bouncing_actions.h common/defs.h common/logger.cpp common/logger.h)