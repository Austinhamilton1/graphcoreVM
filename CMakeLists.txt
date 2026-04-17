cmake_minimum_required(VERSION 3.14)
project(GCVM CXX)

include(FetchContent)

FetchContent_Declare(
    asmjit
    GIT_REPOSITORY  https://github.com/asmjit/asmjit.git
    GIT_TAG         master
)

FetchContent_MakeAvailable(asmjit)

add_library(graphcore SHARED src/graphcore.cpp)
target_include_directories(graphcore PRIVATE include)
target_link_libraries(graphcore asmjit::asmjit)

add_executable(gcvmasm src/gcvmasm.cpp)
target_include_directories(gcvmasm PRIVATE include)

add_executable(graphasm src/graphasm.cpp)
target_include_directories(graphasm PRIVATE include)

add_executable(gcvm src/gcvm.cpp)
target_include_directories(gcvm PRIVATE include)
target_link_libraries(gcvm PRIVATE graphcore)