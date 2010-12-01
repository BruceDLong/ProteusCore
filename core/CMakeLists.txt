cmake_minimum_required(VERSION 2.8)

add_definitions(-fpermissive -Wno-deprecated -g)

add_library(Proteus SHARED Proteus.cpp InfonIO.cpp)

add_executable(ptest ptest.cpp)

target_link_libraries(ptest Proteus)
