set(LIB_NAME ecsLib)
set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_SOURCES )

include_directories(${LIB_PATH}/../../include)

file(GLOB_RECURSE LIB_SOURCES ${LIB_PATH}/*.cpp)

add_library(${LIB_NAME} STATIC ${LIB_SOURCES})
