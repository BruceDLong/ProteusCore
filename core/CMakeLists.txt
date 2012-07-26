cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

add_definitions(-Wno-deprecated -Wno-write-strings -fpermissive -g -Wall)

set(PROTEUS_SRCS Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp)
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)

include_directories( ../builds/libs/GMP_BigInts/gmp-5.0.4/include/ ../builds/libs/unicode/release-49-1-1/include/)
link_directories(../builds/libs/GMP_BigInts/gmp-5.0.4/lib/ ../builds/libs/unicode/release-49-1-1/lib/)

#add_library(Proteus SHARED ${PROTEUS_SRCS})
#add_executable(ptest ptest.cpp ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip readline gmpxx gmp ${UNICODE_LIBS})
#target_link_libraries(ptest gmp gmpxx ${UNICODE_LIBS})
