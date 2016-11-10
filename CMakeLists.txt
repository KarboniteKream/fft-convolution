cmake_minimum_required(VERSION 3.6)
project(fft-convolution)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(SOURCE_FILES main.cpp)
add_executable(fft-convolution ${SOURCE_FILES})
target_link_libraries(fft-convolution pthread)
