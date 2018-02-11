cmake_minimum_required(VERSION 2.8)
project(NBSfM)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_executable(NBSfM NBSfM.cpp)
target_link_libraries(NBSfM ${OpenCV_LIBS})
