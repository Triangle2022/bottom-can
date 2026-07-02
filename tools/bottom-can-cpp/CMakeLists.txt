cmake_minimum_required(VERSION 3.16)

project(bottom_can_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(bottom-can-cpp src/main.cpp)

if(APPLE OR UNIX)
  target_compile_options(bottom-can-cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

