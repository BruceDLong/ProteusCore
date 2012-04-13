cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0015 NEW)

add_definitions(-Wno-deprecated -Wno-write-strings -fpermissive -g -Wall)

set(PROTEUS_SRCS Proteus.cpp InfonIO.cpp Functions.cpp)
set(UNICODE_LIBS icuuc icuio icudata icutu icui18n icule iculx icutest dl)

include_directories( ../unicode/install/include/ )

link_directories(../libfixmath/libfixmath ../unicode/install/lib /usr/lib /usr/lib/i386-linux-gnu/)
#add_library(Proteus SHARED ${PROTEUS_SRCS})

add_executable(ptest ptest.cpp ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip readline fixmath ${UNICODE_LIBS})
target_link_libraries(ptest fixmath ${UNICODE_LIBS})
