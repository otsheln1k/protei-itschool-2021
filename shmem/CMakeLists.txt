cmake_minimum_required(VERSION 3.1)

project(shmem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_SOURCES
  src/shmem.cpp
  src/common.cpp
)

add_compile_options(
  -Wall
  -Wextra
)

add_executable(send
  src/send.cpp
  ${COMMON_SOURCES}
)
target_link_libraries(send
  PRIVATE -lrt
  PRIVATE -lpthread
)

add_executable(recv
  src/recv.cpp
  ${COMMON_SOURCES}
)
target_link_libraries(recv
  PRIVATE -lrt
  PRIVATE -lpthread
)
