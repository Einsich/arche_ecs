cmake_minimum_required(VERSION 3.0)
project(ecs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-m64 -Wall -Wextra -Wno-pragma-pack -Wno-deprecated-declarations -g")

add_subdirectory(${CMAKE_SOURCE_DIR}/sources/ecs)
add_subdirectory(${CMAKE_SOURCE_DIR}/sources/tests)
