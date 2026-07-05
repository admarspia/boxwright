cmake_minimum_required(VERSION 3.16)
project(sandbox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sandbox src/main.cpp src/sandbox.cpp)
