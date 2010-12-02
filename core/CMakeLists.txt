cmake_minimum_required(VERSION 2.6)

add_definitions(-Wno-deprecated -g -Wall)

add_library(Proteus SHARED Proteus.cpp InfonIO.cpp)

add_executable(ptest ptest.cpp)

target_link_libraries(ptest Proteus)
