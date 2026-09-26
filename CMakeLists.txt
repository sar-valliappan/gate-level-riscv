cmake_minimum_required(VERSION 3.16)
project(gate-level-riscv CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(gate-level-riscv src/main.cpp)
