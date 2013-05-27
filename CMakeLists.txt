# TOP-LEVEL Slipstream CMakeLists.txt
# Creates Libraries then cmakes core and slip

cmake_minimum_required(VERSION 2.8.5)

option(MAKE_SDL "Build our own SDL and SDL related libs" ON)
option(MAKE_GMP "Build our own math (gmp) and related libs" ON)
option(MAKE_CPF "Build our own cairo/pango/freetype libs" ON)
option(MAKE_SQL "Build our own sqlite lib" ON)
option(MAKE_RDL "Build our own Readline lib" ON)
option(MAKE_ICU "Build our own unicode libs" ON)

set(ICU_UnicodeVersion release-51-1)
set(GMP_Version gmp-5.0.4)

SET(CMAKE_COLOR_MAKEFILE ON)
#SET(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (Slipstream)

set(toolChainPath ${CMAKE_CURRENT_SOURCE_DIR}/xTools})

if (NOT DEFINED PLAT_BUILD) 
	SET(PLAT_BUILD "HOST")
endif()

if (${PLAT_BUILD} STREQUAL "HOST")
	message(STATUS "Building for local host.")


elseif (${PLAT_BUILD} STREQUAL xLINUX_i386)
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/i368-linux-gnu-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/i368-linux-gnu-g++)
	SET(xCONFIG_VARS " ABI=32 CXXFLAGS=-m32 CFLAGS=-m32 LDFLAGS=-m32 ")
	SET(ICU_PLAT_CODE linux)  # Set to option given by 'runConfigureICU --help'

elseif (${PLAT_BUILD} STREQUAL "xLINUX_x86-64")
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/x86_64-linux-gnu-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/x86_64-linux-gnu-g++)
	SET(xCONFIG_VARS "")
	SET(ICU_PLAT_CODE linux)  # Set to option given by 'runConfigureICU --help'

elseif (${PLAT_BUILD} STREQUAL "xLINUX_ARM")
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(ICU_PLAT_CODE linux)  # Set to option given by 'runConfigureICU --help'

elseif (${PLAT_BUILD} STREQUAL "xANDROID_ARM") # android_14
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/arm-linux-androideabi-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/arm-linux-androideabi-g++)
	SET(xCONFIG_VARS " CXXFLAGS=-marm CFLAGS=-marm LDFLAGS=-lstdc++ ")
	SET(xCONFIG_GMP " --host=arm-linux-androideabi ")
	SET(ICU_PLAT_CODE linux)  # Set to option given by 'runConfigureICU --help'
	
elseif (${PLAT_BUILD} STREQUAL "xANDROID_MIPS")
	message(STATUS "Building for ${PLAT_BUILD}.")

elseif (${PLAT_BUILD} STREQUAL "xANDROID_i386")
	message(STATUS "Building for ${PLAT_BUILD}.")
elseif (${PLAT_BUILD} STREQUAL "xiPhone")
	message(STATUS "Building for ${PLAT_BUILD}.")
	
elseif (${PLAT_BUILD} STREQUAL "xOSx")
	message(STATUS "Building for ${PLAT_BUILD}.")
	
elseif (${PLAT_BUILD} STREQUAL "xWINDOWS_i386")
	message(STATUS "Building for ${PLAT_BUILD}.")
	
elseif (${PLAT_BUILD} STREQUAL "xWINDOWS_x86-64")
	message(STATUS "Building for ${PLAT_BUILD}.")
	
else ()
	message(FATAL_ERROR "No Build Settings for ${PLAT_BUILD}.")
	return()
endif ()



# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /home/bruce/dev/toolchains/android_14)


#set(CMAKE_C_FLAGS -m32)
#set(CMAKE_CXX_FLAGS -m32)
include(ExternalProject)
#find_package(Git)
message(STATUS "CMAKE_TOOLCHAIN_FILE='${CMAKE_TOOLCHAIN_FILE}'")

if (MAKE_SDL)
	#find_package(Mercurial)
	#=========================================
	# Build SDL
	ExternalProject_Add(
	  SDL_LIBS
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/
	  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL
	  UPDATE_COMMAND ""

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/src/SDL
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/src/SDL/configure  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}  --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/SDL/1.3

	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)

	#=========================================
	# Build SDL_Image
	ExternalProject_Add(
	  SDL_Image


	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL_image/
	  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL_image/
	  UPDATE_COMMAND ""

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL_image/src/SDL_image
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sdl_image_config.sh "${CMAKE_CURRENT_BINARY_DIR}"
	#  #export SDL_CONFIG=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/SDL/1.3/bin/sdl2-config && ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL_image/src/SDL_image/configure ABI=32  CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32  --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/SDL_image
	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 1
	  DEPENDS "SDL_LIBS"
	)
endif (MAKE_SDL)


if (MAKE_CPF)
	#=========================================
	# Build Cairo

	#=========================================
	# Build Pango

	#=========================================
	# Build Freetype2???
endif (MAKE_CPF)

if (MAKE_GMP)
	#=========================================
	# Build GMP
	ExternalProject_Add(
	  GMP_BigInts
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/${GMP_Version}.tar.bz2"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/src/${GMP_Version}/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/src/${GMP_Version}/configure ${xCONFIG_GMP} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-cxx --enable-static --disable-shared --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/GMP_BigInts/${GMP_Version}/
	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)

	#=========================================
	# Build MPFA
	#ExternalProject_Add(
	  #MPFA_Floats
	  #PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/MPFA_Floats/
	  #URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/mpfr-3.1.0.tar.bz2"

	  #SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/
	  #CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/configure  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}  --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/MPFA_Floats/mpfr-3.1.0/ --with-gmp=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/GMP_BigInts/${GMP_Version}/

	  #BUILD_COMMAND  $(MAKE)
	  #INSTALL_COMMAND  $(MAKE) install
	  #BUILD_IN_SOURCE 0
	  #DEPENDS "GMP_BigInts"
	#)
endif (MAKE_GMP)


if (MAKE_SQL)
	#=========================================
	# Build sqlite3
	ExternalProject_Add(
	  sqlite3DB
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/sqlite-autoconf-3071602.tar.gz"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/src/sqlite-autoconf-3071602/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/src/sqlite-autoconf-3071602/configure  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-cxx --enable-static --disable-shared --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/sqlite3/sqlite-autoconf-3071602/

	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)
endif (MAKE_SQL)


if (MAKE_RDL)
	#=========================================
	# Build Readline
	ExternalProject_Add(
	  Readline
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/readline-6.2.tar.gz"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/src/readline-6.2/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/src/readline-6.2/configure  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-static --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Readline/readline-6.2/

	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)
endif (MAKE_RDL)


if (MAKE_ICU)
	#=========================================
	# Build ICU (unicode)
	find_package(Subversion)
	ExternalProject_Add(
	  ICU_unicode
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/unicode/
	  SVN_REPOSITORY "http://source.icu-project.org/repos/icu/icu/tags/${ICU_UnicodeVersion}"
	  UPDATE_COMMAND ""
	  # TODO: patch uversion.h and platform.h as per readme.html
	  PATCH_COMMAND
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/unicode/${ICU_UnicodeVersion}
	  
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/unicode/${ICU_UnicodeVersion}/source/runConfigureICU ${ICU_PLAT_CODE}  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --with-data-packaging=archive --with-library-bits=32  --enable-static  --disable-shared --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/unicode/${ICU_UnicodeVersion}

	  BUILD_COMMAND $(MAKE)
	  INSTALL_COMMAND $(MAKE) install
	  BUILD_IN_SOURCE 0
	)
endif (MAKE_ICU)


add_subdirectory (core)
add_subdirectory (slip)
