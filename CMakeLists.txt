cmake_minimum_required(VERSION 3.14)
project(GCVM CXX)

find_package(OpenMP REQUIRED)

add_library(gcvm SHARED src/gcvm.cpp)
target_include_directories(gcvm PRIVATE include)
target_link_libraries(gcvm PRIVATE OpenMP::OpenMP_CXX)

add_executable(gcvmasm src/assembler.cpp)
target_include_directories(gcvmasm PRIVATE include)

add_executable(test test/test.cpp)
target_include_directories(test PRIVATE include)
target_link_libraries(test PRIVATE gcvm)

add_executable(pagerank examples/pagerank.cpp)
target_include_directories(pagerank PRIVATE include)
target_link_libraries(pagerank PRIVATE gcvm)