cmake_minimum_required(VERSION 2.6)

add_definitions(-Wno-deprecated -Wno-write-strings -fpermissive -g -Wall)

set(PROTEUS_SRCS Proteus.cpp InfonIO.cpp)

#add_library(Proteus SHARED ${PROTEUS_SRCS})

add_executable(ptest ptest.cpp ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip readline)

# target_link_libraries(ptest Proteus)
