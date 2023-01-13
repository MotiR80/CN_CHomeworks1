cmake_minimum_required(VERSION 3.9.1)

PROJECT(FTP_SERVER)

set(CMAKE_CXX_STANDARD 14)

find_package(jsoncpp CONFIG REQUIRED)

add_executable(FTP_SERVER main.cpp)

target_link_libraries(FTP_SERVER PRIVATE jsoncpp_object jsoncpp_static JsonCpp::JsonCpp)

