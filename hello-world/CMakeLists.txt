cmake_minimum_required(VERSION 3.1)

project(hello-world LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
  -Wall
  -Wextra
)

add_executable(hello
  src/hello.cpp
)
