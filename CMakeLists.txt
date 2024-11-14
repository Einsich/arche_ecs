cmake_minimum_required(VERSION 3.0)
project(ecs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-m64 -Wall -Wextra -Wno-pragma-pack -Wno-deprecated-declarations -g")


add_subdirectory(${CMAKE_SOURCE_DIR}/sources/ecs)


include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(ecs main.cpp)

target_link_libraries(ecs ecsLib)
