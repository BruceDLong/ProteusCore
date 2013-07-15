cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

add_definitions(-std=gnu++11 -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall -Wno-comment -pthread `pkg-config --cflags glib-2.0`)

set(HomeDir ${CMAKE_CURRENT_BINARY_DIR})

set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)
set(PANGO_INCLUDE_DIRS /usr/include/pango-1.0/ /usr/include/cairo /usr/include/glib-2.0/ 
                       /usr/lib/i386-linux-gnu/glib-2.0/include /usr/include/pixman-1 
					   /usr/include/freetype2 /usr/include/libpng12)
set(PANGO_LIBS pangocairo-1.0 pango-1.0 cairo gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0)

include_directories(  ${HomeDir}/../builds/libs/SDL/1.3/include/ 
					  ${HomeDir}/../builds/libs/SDL/1.3/include/SDL2/ 
					  ${HomeDir}/../builds/libs/SDL_image/include/
			   		  ${HomeDir}/../builds/libs/GMP_BigInts/${GMP_Version}/include/
					  ${HomeDir}/../builds/libs/sqlite3/sqlite-autoconf-3071602/include/
					  ${HomeDir}/../builds/libs/unicode/${ICU_UnicodeVersion}/include/
					  ${HomeDir}/../builds/libs/GIT_LIB/libgit2-0.18.0/include/
				      ${PANGO_INCLUDE_DIRS}
					  ${Slipstream_SOURCE_DIR}/deps/archive/
					)

link_directories(
				 ${HomeDir}/../core
                 ${HomeDir}/../builds/libs/GMP_BigInts/${GMP_Version}/lib/
                 ${HomeDir}/../builds/libs/unicode/${ICU_UnicodeVersion}/lib/
                 ${HomeDir}/../builds/libs/SDL_image/lib/ 
                 ${HomeDir}/../builds/libs/SDL/1.3/lib
                 ${HomeDir}/../builds/libs/sqlite3/sqlite-autoconf-3071602/lib/
                 ${HomeDir}/../builds/libs/GIT_LIB/libgit2-0.18.0/lib/
				)

add_executable(slip slip.cpp )
target_link_libraries(slip Proteus git2 gmp gmpxx  pthread sqlite3 ${UNICODE_LIBS} SDL2_image SDL2 ${PANGO_LIBS})
