cmake_minimum_required(VERSION 3.0)
project(link_test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(lib/link/AbletonLinkConfig.cmake)

add_executable(link_test main.cpp)

include_directories(${link_HEADERS})

target_link_libraries(link_test pthread)
target_link_libraries(link_test Ableton::Link)
target_link_libraries(link_test wiringPi)
