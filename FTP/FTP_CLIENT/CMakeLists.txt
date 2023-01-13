cmake_minimum_required(VERSION 3.24)
project(FTP_CLIENT)

set(CMAKE_CXX_STANDARD 14)

find_package(jsoncpp CONFIG REQUIRED)

add_executable(FTP_CLIENT main.cpp)

target_link_libraries(FTP_CLIENT PRIVATE jsoncpp_object jsoncpp_static JsonCpp::JsonCpp)
