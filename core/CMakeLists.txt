cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

set(LANG_MODULES XlaterENGLISH.cpp)

add_definitions(-m32 -std=gnu++11 -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall)

set(PROTEUS_SRCS Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp ${LANG_MODULES})
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)

include_directories( #../builds/libs/GMP_BigInts/gmp-5.0.4/include/ 
					../builds/libs/unicode/release-49-1-1/include/ 
					../../dev/boost_1_51_0/ 
					#../builds/libs/Readline/readline-6.2/include/
					)
link_directories(#../builds/libs/GMP_BigInts/gmp-5.0.4/lib/ 
				 ../builds/libs/unicode/release-49-1-1/lib/ 
				# ../builds/libs/Readline/readline-6.2/lib/
				 )

#add_library(Proteus SHARED ${PROTEUS_SRCS})
add_executable(clip clip.cpp ${PROTEUS_SRCS})
target_link_libraries(clip readline gmpxx gmp ${UNICODE_LIBS})
set_target_properties(clip PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
