@echo off
set CMAKE_BUILD_TYPE=%1
set BUILD_DIR=%2

pushd codegen
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -B ../bin/codegen/%BUILD_DIR%
popd

pushd bin\codegen\%BUILD_DIR%
ninja
popd