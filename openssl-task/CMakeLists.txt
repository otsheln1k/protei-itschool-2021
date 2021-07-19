cmake_minimum_required(VERSION 3.1)

project(openssl-task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
  -Wall
  -Wextra
)

set(PROGNAME calcdigest)

find_package(OpenSSL REQUIRED)

add_executable("${PROGNAME}"
  src/main.cpp
)
target_include_directories("${PROGNAME}"
  PRIVATE "${OPENSSL_INCLUDE_DIRS}"
)
target_link_libraries("${PROGNAME}"
  PRIVATE "${OPENSSL_LIBRARIES}"
)
