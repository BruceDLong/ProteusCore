# TOP-LEVEL Slipstream CMakeLists.txt
# Creates Libraries then cmakes core and slip

cmake_minimum_required(VERSION 2.8.5)
SET(CMAKE_COLOR_MAKEFILE ON)
#SET(CMAKE_VERBOSE_MAKEFILE OFF)

set(ICU_UnicodeVersion release-49-1-1)

include(ExternalProject)

#find_package(Mercurial)
find_package(Subversion)
find_package(Git)

set($ENV{SDL_CONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/SDL/1.3/bin/sdl2-config")

#=========================================
# Build SDL
ExternalProject_Add(
  SDL_LIBS
  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/
  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL
  UPDATE_COMMAND ""

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/src/SDL
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/src/SDL/configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/SDL/1.3

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build SDL_Image
ExternalProject_Add(
  SDL_Image


  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_image/
  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL_image/
  UPDATE_COMMAND ""

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_image/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_image/src/SDL_image/configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/SDL_image
  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
  DEPENDS "SDL_LIBS"
)

#=========================================
# Build Cairo

#=========================================
# Build Pango

#=========================================
# Build Freetype2???

#=========================================
# Build GMP
ExternalProject_Add(
  GMP_BigInts
  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/GMP_BigInts/
  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/gmp-5.0.4.tar.bz2"

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/GMP_BigInts/src/gmp-5.0.4/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/GMP_BigInts/src/gmp-5.0.4/configure --enable-fat --enable-cxx --disable-shared --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/GMP_BigInts/gmp-5.0.4/

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build MPFA
ExternalProject_Add(
  MPFA_Floats
  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/
  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/mpfr-3.1.0.tar.bz2"

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/MPFA_Floats/mpfr-3.1.0/ --with-gmp=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/GMP_BigInts/gmp-5.0.4/

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
  DEPENDS "GMP_BigInts"
)

#=========================================
# Build Readline
ExternalProject_Add(
  Readline
  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/Readline/
  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/readline-6.2.tar.gz"

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Readline/src/readline-6.2/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/Readline/src/readline-6.2/configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/Readline/readline-6.2/

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build ICU (unicode)
ExternalProject_Add(
  ICU_unicode
  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/unicode/
  SVN_REPOSITORY "http://source.icu-project.org/repos/icu/icu/tags/${ICU_UnicodeVersion}"
#  SVN_REVISION "release-49-1-1"
#  GIT_REPOSITORY git://github.com/commontk/CTK.git
#  GIT_TAG origin/master
  UPDATE_COMMAND ""
  # TODO: patch uversion.h and platform.h as per readme.html
  PATCH_COMMAND
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/unicode/${ICU_UnicodeVersion}
  # TODO: for cross compiling, the 'linux' in the following should be one of those given by 'runConfigureICU --help'
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/unicode/${ICU_UnicodeVersion}/source/runConfigureICU Linux --with-data-packaging=archive --enable-static --disable-shared --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/unicode/release-49-1-1

  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND $(MAKE) install
  BUILD_IN_SOURCE 0
)

add_subdirectory (core)
#add_subdirectory (slip)
