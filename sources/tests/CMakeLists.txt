
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/sources/tests)

add_executable(benchmark
  benchmark/main.inl.cpp
)

add_executable(unit_tests
  unit_tests/main.inl.cpp
)

target_link_libraries(benchmark ecsLib)
target_link_libraries(unit_tests ecsLib)
