cmake_minimum_required(VERSION 3.6)

project(OSS13)

set(CMAKE_CXX_STANDARD 17)

include_directories(External/plog/include)
include_directories(External/sfml-imgui)

add_subdirectory("SharedLibrary")
add_subdirectory("OSS13 Client")
add_subdirectory("OSS13 Server")
