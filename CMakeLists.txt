# TOP-LEVEL Slipstream CMakeLists.txt
# Creates Libraries then cmakes core and slip
# Use script from crosstool-ng.org to create cross-compile tool-chains.
# Creates packaged installers for other platforms using cpack

SET(CMAKE_SYSTEM_NAME x86-Linux)
#SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ~/AwesomeWorld/XCompilers/x-tools/i386-unknown-elf/bin/)
#SET(CMAKE_CXX_COMPILER /opt/eldk-2007-01-19/usr/bin/ppc_74xx-g++)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  ~/AwesomeWorld/XCompilers/config)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


cmake_minimum_required(VERSION 2.8.5)
PROJECT (Turbulence)
SET(CMAKE_COLOR_MAKEFILE ON)
SET(PLATFORM ${CMAKE_SYSTEM_NAME})

set(ICU_UnicodeVersion release-49-1-1)

SET(ep_base ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM})
set_property(DIRECTORY PROPERTY "EP_BASE" ${ep_base})

include(ExternalProject)

#find_package(Mercurial)
find_package(Subversion)
find_package(Git)

set($ENV{SDL_CONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/SDL/1.3/bin/sdl2-config")


#=========================================
# Build SDL
ExternalProject_Add(
  SDL_LIBS
  #SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/SDL/)
  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL
  UPDATE_COMMAND ""

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/src/SDL
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL/src/SDL/configure --prefix=${ep_base}/Install

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build SDL_Image
ExternalProject_Add(
  SDL_Image


 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/SDL_image/)
  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL_image/
  UPDATE_COMMAND ""

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_image/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL_image/src/SDL_image/configure --prefix=${ep_base}/Install
  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
  DEPENDS "SDL_LIBS"
)

#=========================================
# Build Pixman
ExternalProject_Add(
  Pixman
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/Pixman)
  DOWNLOAD_COMMAND git clone git://anongit.freedesktop.org/git/pixman.git
  UPDATE_COMMAND ""

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Pixman
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/Pixman/src/pixman/autogen.sh --prefix=${ep_base}/Install
  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build Cairo
ExternalProject_Add(
  Cairo
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/Cairo)
  DOWNLOAD_COMMAND git clone git://anongit.freedesktop.org/git/cairo
  UPDATE_COMMAND ""

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Cairo
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/Cairo/src/cairo/autogen.sh --prefix=${ep_base}/Install
  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
  DEPENDS "Pixman"
)

#=========================================
# Build Pango
ExternalProject_Add(
  Pango
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/Pango)
  DOWNLOAD_COMMAND git clone git://git.gnome.org/pango
  UPDATE_COMMAND ""

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Pango
  CONFIGURE_COMMAND PKG_CONFIG_PATH=${CMAKE_CURRENT_SOURCE_DIR}/deps/Cairo/src/Cairo-build/src/cairo.pc/path ${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/Pango/src/pango/autogen.sh --prefix=${ep_base}/Install
  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
  DEPENDS "Cairo"
#sudo apt-get install libgtk2.0-dev
#sudo apt-get install gtk+-2.0
)



#=========================================
# Build libpng-dev
#temporary solution=sudo apt-get install libpng12-dev

#=========================================
# Build Freetype2???

#=========================================
# Build GMP
ExternalProject_Add(
  GMP_BigInts
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/GMP_BigInts/)
  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/gmp-5.0.4.tar.bz2"

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/GMP_BigInts/src/gmp-5.0.4/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/GMP_BigInts/src/gmp-5.0.4/configure --enable-fat --enable-cxx --disable-shared --prefix=${ep_base}/Install

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build MPFA
#ExternalProject_Add(
#  MPFA_Floats
#  PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/
#  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/mpfr-3.1.0.tar.bz2"
#
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/
#  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/MPFA_Floats/mpfr-3.1.0/ --with-gmp=${CMAKE_CURRENT_SOURCE_DIR}/builds/libs/GMP_BigInts/gmp-5.0.4/
#
#  BUILD_COMMAND  $(MAKE)
#  INSTALL_COMMAND  $(MAKE) install
#  BUILD_IN_SOURCE 0
#  DEPENDS "GMP_BigInts"
#)

#=========================================
# Build Readline
ExternalProject_Add(
  Readline
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/Readline/)
  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/readline-6.2.tar.gz"

  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Readline/src/readline-6.2/
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/Readline/src/readline-6.2/configure --prefix=${ep_base}/Install

  BUILD_COMMAND  $(MAKE)
  INSTALL_COMMAND  $(MAKE) install
  BUILD_IN_SOURCE 0
)

#=========================================
# Build ICU (unicode)
ExternalProject_Add(
  ICU_unicode
 # SET(EP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/unicode/)
  SVN_REPOSITORY "http://source.icu-project.org/repos/icu/icu/tags/${ICU_UnicodeVersion}"
#  SVN_REVISION "release-49-1-1"
#  GIT_REPOSITORY git://github.com/commontk/CTK.git
#  GIT_TAG origin/master
  UPDATE_COMMAND ""
  # TODO: patch uversion.h and platform.h as per readme.html
  PATCH_COMMAND
  #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/unicode/${ICU_UnicodeVersion}
  # TODO: for cross compiling, the 'linux' in the following should be one of those given by 'runConfigureICU --help'
  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deps/unicode/${ICU_UnicodeVersion}/source/runConfigureICU Linux --with-data-packaging=archive --enable-static --disable-shared --prefix=${ep_base}/Install

  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND $(MAKE) install
  BUILD_IN_SOURCE 0
)

add_subdirectory (core)
add_subdirectory (slip)

INSTALL(FILES ./slip/darkTheme1.pr ./slip/world.pr ./slip/users.pr ./slip/brucesStuff.pr ./slip/AbstractBkgnd.png ./slip/FreeSans.ttf DESTINATION .)
INSTALL (PROGRAMS ./slip/slip DESTINATION .)

#####################################################################

#CPack here

INCLUDE(InstallRequiredSystemLibraries)

SET (CPACK_DEBIAN_PACKAGE_NAME "Turbulence")
SET (CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY "Enter the Dragon")
SET (CPACK_DEBIAN_PACKAGE_MAINTAINER "bruce@infomage.com")
SET (CPACK_DEBIAN_PACKAGE_VERSION "0.1")
SET (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386") #if not specified will be set to "i386"
SET (CPACK_DEBIAN_PACKAGE_SECTION "") #if not specified will be set to "devel"
SET (CPACK_DEBIAN_PACKAGE_PRIORITY "") # if not specified will be set to "optional"
SET (CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
SET (CPACK_DEBIAN_PACKAGE_SUGGESTS "")
SET (CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "") #This variable allow advanced user to add custom script to the control.tar.gz (inside the .deb archive) ex. ${CMAKE_CURRENT_SOURCE_DIR}/postinst

SET (CPACK_GENERATOR DEB)
INCLUDE(CPack)


