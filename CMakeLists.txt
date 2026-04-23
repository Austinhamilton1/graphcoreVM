cmake_minimum_required(VERSION 3.14)
project(GCVM CXX)

include(FetchContent)

FetchContent_Declare(
    asmjit
    GIT_REPOSITORY  https://github.com/asmjit/asmjit.git
    GIT_TAG         master
)

FetchContent_MakeAvailable(asmjit)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY  https://github.com/jbeder/yaml-cpp.git
    GIT_TAG         master
)

FetchContent_MakeAvailable(yaml-cpp)

add_library(graphcore SHARED 
    src/graphcore.cpp 
    src/jit.cpp
)
target_include_directories(graphcore PRIVATE include)
target_link_libraries(graphcore asmjit::asmjit)

add_executable(gcvm src/gcvm.cpp)
target_include_directories(gcvm PRIVATE include)
target_link_libraries(gcvm PRIVATE graphcore yaml-cpp::yaml-cpp)