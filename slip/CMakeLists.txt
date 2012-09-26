cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0015 NEW)

set(LANG_MODULES XlaterENGLISH.cpp)

add_definitions(-std=gnu++0x -Wno-deprecated -Wno-write-strings -fpermissive -g -Wall -pthread)

set(SLIP_SRCS slip.cpp Proteus.cpp convs.cpp InfonIO.cpp Functions.cpp ${LANG_MODULES})
set(UNICODE_LIBS icuio icui18n icutu icuuc icudata icule iculx icutest dl)
set(PANGO_INCLUDE_DIRS /usr/include/pango-1.0/ /usr/include/cairo /usr/include/glib-2.0/ /usr/lib/i386-linux-gnu/glib-2.0/include /usr/include/pixman-1 /usr/include/freetype2 /usr/include/libpng12)

include_directories(  ../builds/libs/SDL/1.3/include/ ../builds/libs/SDL/1.3/include/SDL2/ ../builds/libs/SDL_image/include/ ../builds/libs/GMP_BigInts/gmp-5.0.4/include/ ../builds/libs/unicode/release-49-1-1/include/  /usr/include/pango-1.0/  /usr/include/cairo /usr/include/glib-2.0/ /usr/lib/i386-linux-gnu/glib-2.0/include /usr/include/pixman-1 /usr/include/freetype2 /usr/include/libpng12  )


link_directories(../builds/libs/GMP_BigInts/gmp-5.0.4/lib/ ../builds/libs/unicode/release-49-1-1/lib/ ../builds/libs/SDL_image/lib/ ../builds/libs/SDL/1.3/lib)

#add_library(Proteus SHARED ${SLIP_SRCS})

add_executable(slip  slip.cpp ../core/Proteus.cpp ../core/convs.cpp ../core/InfonIO.cpp ../core/Functions.cpp ../core/XlaterENGLISH.cpp)

target_link_libraries(slip gmp gmpxx pthread SDL2_image SDL2 ${UNICODE_LIBS} pangocairo-1.0 pango-1.0 cairo gobject-2.0 gmodule-2.0 gthread-2.0 rt glib-2.0)

