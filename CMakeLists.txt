cmake_minimum_required(VERSION 3.14)
project(GCVM CXX)

find_package(OpenMP REQUIRED)

add_library(graphcore SHARED src/graphcore.cpp)
target_include_directories(graphcore PRIVATE include)
target_link_libraries(graphcore PRIVATE OpenMP::OpenMP_CXX)

add_executable(gcvmasm src/gcvmasm.cpp)
target_include_directories(gcvmasm PRIVATE include)

add_executable(graphasm src/graphasm.cpp)
target_include_directories(graphasm PRIVATE include)

add_executable(gcvm src/gcvm.cpp)
target_include_directories(gcvm PRIVATE include)
target_link_libraries(gcvm PRIVATE graphcore)