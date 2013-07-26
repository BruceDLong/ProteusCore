# TOP-LEVEL Slipstream CMakeLists.txt
# Creates Libraries then cmakes core and slip

cmake_minimum_required(VERSION 2.8.5)

set(MAKE_SDL ON) #  "Build our own SDL lib"
set(MAKE_IMG ON) #  "Build our own image display (png, jpg, etc) libs"
set(MAKE_GMP ON) #  "Build our own math (gmp) and related libs"
set(MAKE_CPF ON) #  "Build our own cairo/pango/freetype libs"
set(MAKE_SQL ON) #  "Build our own sqlite lib"
set(MAKE_RDL ON) #  "Build our own Readline lib"
set(MAKE_GIT ON) #  "Build our own GIT lib"
set(MAKE_ICU ON) #  "Build our own unicode libs"
set(MAKE_PRG ON) #  "Build the proteus slip"

set(ICU_UnicodeVersion release-51-1)
set(GMP_Version gmp-5.0.4)

SET(CMAKE_COLOR_MAKEFILE ON)
#SET(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (Slipstream)

set(toolChainPath ${CMAKE_CURRENT_SOURCE_DIR}/xTools)
set(LOCAL_CC ${CMAKE_C_COMPILER})
set(LOCAL_CXX ${CMAKE_CXX_COMPILER})

set(ICU_DIR unicode)

if (NOT DEFINED PLAT_BUILD) 
	SET(PLAT_BUILD "HOST")
endif()
set(CROSS_COMPILE 'yes')
if (${PLAT_BUILD} STREQUAL "HOST")
	message(STATUS "Building for local host. (${CMAKE_HOST_SYSTEM_PROCESSOR})")
	set(CROSS_COMPILE 'no')
	set(MAKE_CPF OFF)
	if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
		SET(xCONFIG_GMP  "--enable-fat")
		SET(xCONFIG_VARS "CXXFLAGS=-I./builds/libs/Freetype/include/")
	elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "i386")
		SET(xCONFIG_GMP  "--enable-fat")
		SET(xCONFIG_VARS "CXXFLAGS=-I./builds/libs/Freetype/include/")
	endif()
	SET(ICU_PLAT_CODE_local Linux)

elseif (${PLAT_BUILD} STREQUAL "xLINUX_i386")
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/i368-linux-gnu-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/i368-linux-gnu-g++)
	SET(xCONFIG_VARS "ABI=32 CXXFLAGS=-m32 CFLAGS=-m32 LDFLAGS=-m32")
	SET(ICU_PLAT_CODE Linux)
	SET(xCONFIG_ICU "--with-library-bits=32")

elseif (${PLAT_BUILD} STREQUAL "xLINUX_x86-64")
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/x86_64-linux-gnu-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/x86_64-linux-gnu-g++)
	SET(xCONFIG_VARS "")
	SET(ICU_PLAT_CODE Linux)

elseif (${PLAT_BUILD} STREQUAL "xLINUX_ARM")
	message(STATUS "Building for ${PLAT_BUILD}.")
	SET(ICU_PLAT_CODE Linux)

elseif (${PLAT_BUILD} STREQUAL "xANDROID_ARM") # android_14
	message(STATUS "Building for ${PLAT_BUILD}.")
	set(MAKE_SDL 'OFF')
	set(MAKE_RDL 'OFF')
	set(MAKE_PRG 'OFF')
	
	SET(CMAKE_C_COMPILER   ${toolChainPath}/${PLAT_BUILD}/bin/arm-linux-androideabi-gcc)
	SET(CMAKE_CXX_COMPILER ${toolChainPath}/${PLAT_BUILD}/bin/arm-linux-androideabi-g++)
	SET(xCONFIG_GMP "--host=arm-android-linux  CC_FOR_BUILD=${LOCAL_CC}")
	SET(xCONFIG_FC --host=arm-linux  CC_FOR_BUILD=${LOCAL_CC} --with-arch=arm --sysconfdir=${toolChainPath}/${PLAT_BUILD}/)
	SET(xCONFIG_RDL "--host=arm-eabi")
	SET(xCONFIG_SQL "--host=arm-eabi")
	SET(xCONFIG_SDL "--host=arm-android-linux")
	SET(xCONFIG_CAIRO --disable-xlib --disable-xcb --disable-xlib-xcb 
	      CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/freetype2 
	      "LDFLAGS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT/lib/libexpat.so -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Remiss/libremiss.a"
	      FREETYPE_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/ 
	      FREETYPE_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/lib/libfreetype.a 
	      FONTCONFIG_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FONTCONFIG/fontconfig-2.10.93/include/ 
	      FONTCONFIG_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FONTCONFIG/fontconfig-2.10.93/lib/libfontconfig.a 
	      pixman_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Pixman/include/pixman-1/ 
	      pixman_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Pixman/lib/libpixman-1.a 
	      png_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PNG_LIB/libpng-1.6.2/include/ 
	      png_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PNG_LIB/libpng-1.6.2/lib/libpng.a
	      "GOBJECT_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/include/glib-2.0 -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/glib-2.0/include"
	      GOBJECT_LIBS=-L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib
		  glib_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/include
		  "glib_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/libglib-2.0.so -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/libgobject-2.0.so"
	      )
	SET(xCONFIG_PANGO
	      "CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/freetype2 -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Cairo/include"
	      "LDFLAGS=-lgnustl_shared  -L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT/lib/libexpat.so  -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Remiss/libremiss.a"
	      "FREETYPE_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/ -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/freetype2/freetype"
	      "FREETYPE_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/lib/libfreetype.so"
	      "FONTCONFIG_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FONTCONFIG/fontconfig-2.10.93/include/"
	      "FONTCONFIG_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FONTCONFIG/fontconfig-2.10.93/lib/libfontconfig.a"
	      "CAIRO_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Cairo/include/cairo"
	      "CAIRO_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Cairo/lib/libcairo.so"
		  "GLIB_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/glib-2.0/include -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/include/gio-unix-2.0/gio -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/include/glib-2.0"
		  "GLIB_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/libglib-2.0.so -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/libgobject-2.0.so -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib/lib/libgmodule-2.0.so"
	      )	      
	SET(xCONFIG_VARS "CXXFLAGS=-mthumb")
	SET(xCONFIG_PIX --disable-arm-iwmmxt 
	      PNG_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PNG_LIB/libpng-1.6.2/lib/libpng.so 
	      PNG_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PNG_LIB/libpng-1.6.2/include  
	      CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}
	      )
	set(ICU_DIR unicode_util)
	SET(ICU_PLAT_CODE_local Linux)
	SET(ICU_PLAT_CODE Linux --host=arm-android-linux --with-cross-build=${CMAKE_CURRENT_BINARY_DIR}/deps/${ICU_DIR}/${ICU_UnicodeVersion} CPPFLAGS=-nostdlib)

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


if(${CROSS_COMPILE} STREQUAL 'yes')
	# where is the target environment
	SET(CMAKE_FIND_ROOT_PATH  ${toolChainPath}/${PLAT_BUILD})
	# never search for programs in the build host directories
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	# search for libraries and headers in the target directories
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()


message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Local C Compiler: ${LOCAL_CC}")
message(STATUS "Local C++ Compiler: ${LOCAL_CXX}")

include(ExternalProject)

#find_package(Git)
#message(STATUS "CMAKE_TOOLCHAIN_FILE='${CMAKE_TOOLCHAIN_FILE}'")

if ($MAKE_SDL STREQUAL "ON")
	#find_package(Mercurial)
	#=========================================
	# Build SDL
	ExternalProject_Add(
	  SDL_LIBS
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/
	  DOWNLOAD_COMMAND hg clone http://hg.libsdl.org/SDL
	  UPDATE_COMMAND ""

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/src/SDL
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/SDL/src/SDL/configure ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-joystick=no --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/SDL/1.3

	  BUILD_COMMAND  $(MAKE) -j4
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
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sdl_image_config.sh ${xCONFIG_SDL} "\"${CMAKE_CURRENT_BINARY_DIR}\"" "\"CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} \""
	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 1
	  DEPENDS "SDL_LIBS"
	)
endif()

if (${MAKE_IMG} STREQUAL "ON")
	#=========================================
	# Build libJpeg
	ExternalProject_Add(
	  JPEG_LIB
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/JPEG_LIB/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/jpegsrc.v9.tar.gz"
	  
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/JPEG_LIB/src/jpeg-9/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/JPEG_LIB/src/jpeg-9/configure ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/JPEG_LIB/jpeg-9
	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)

	#=========================================
	# Build libPNG
	ExternalProject_Add(
	  PNG_LIB
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/PNG_LIB/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/libpng-1.6.2.tar.gz"
	  
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/PNG_LIB/src/libpng-1.6.2/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/PNG_LIB/src/libpng-1.6.2/configure ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}  --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PNG_LIB/libpng-1.6.2
	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)

endif()


if (${MAKE_CPF} STREQUAL "ON")

	#=========================================
	# Build Expat
	  ExternalProject_Add(
		  EXPAT
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/EXPAT/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/expat-2.1.0.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/EXPAT/src/expat-2.1.0/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/EXPAT/src/expat-2.1.0/configure ${xCONFIG_SDL} 
		       CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
	)
	
	#=========================================
	# Build Freetype
	  ExternalProject_Add(
		  Freetype
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Freetype/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/freetype-2.4.12.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Freetype/src/freetype-2.4.12/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Freetype/src/freetype-2.4.12/configure ${xCONFIG_SDL}
				
		       CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
	)
	
	#=========================================
	# Build Fontconfig
	  ExternalProject_Add(
		  FONTCONFIG
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/FONTCONFIG/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/fontconfig-2.10.93.zip"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/FONTCONFIG/src/fontconfig-2.10.93/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/FONTCONFIG/src/fontconfig-2.10.93/autogen.sh 
		       ${xCONFIG_FC} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER}  CC_FOR_BUILD=/usr/bin/gcc ${xCONFIG_VARS} 
		       FREETYPE_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/lib/libfreetype.a
		       "FREETYPE_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/ -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Freetype/include/freetype2"
		       --with-expat=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT/
		       EXPAT_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT/include
		       EXPAT_LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/EXPAT/lib/libexpat.so
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FONTCONFIG/fontconfig-2.10.93
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
		  DEPENDS Freetype EXPAT
	)
	
	#=========================================
	# Build Pixman
	  ExternalProject_Add(
		  Pixman
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Pixman/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/pixman-0.30.0.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Pixman/src/pixman-0.30.0/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Pixman/src/pixman-0.30.0/autogen.sh ${xCONFIG_SDL}  ${xCONFIG_PIX}  
		       CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} 
		       --disable-dependency-tracking --enable-static --disable-shared 
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Pixman
		  BUILD_COMMAND  $(MAKE)
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
		  DEPENDS FONTCONFIG
	)
	
	#=========================================
	# Build iconv
	  ExternalProject_Add(
		  iconv
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/iconv/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/libiconv-1.14.tar.gz"
		  #DOWNLOAD_COMMAND git clone git://git.savannah.gnu.org/libiconv.git
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/iconv/src/libiconv-1.14/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/iconv/src/libiconv-1.14/configure  gl_cv_header_working_stdint_h=yes  --disable-shared --enable-static ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/iconv
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
	)	
	
	#=========================================
	# Build FFI
	  ExternalProject_Add(
		  FFI
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/FFI/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/libffi-3.0.13.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/FFI/src/libffi-3.0.13/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/FFI/src/libffi-3.0.13/configure --disable-shared --enable-static 
		       ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} 
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FFI
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
	)
	
	#=========================================
	# Build gettext
	  ExternalProject_Add(
		  gettext
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/gettext/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/gettext-0.18.2.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/gettext/src/gettext-0.18.2/
		  # I manually changed msginit.c:1084 to fullname ="USER";/* pwd->pw_gecos; */
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/gettext/src/gettext-0.18.2/configure --disable-shared --enable-static 
		  ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} 
		  CPPFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/iconv/include LDFLAGS=-L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/iconv/lib/libconv.a 
		       --without-emacs --without-git --disable-java --disable-c++ --disable-curses --disable-libasprintf --disable-rpath --enable-relocatable 
		       --without-bzip2 --without-xz --without-included-regex --disable-nls
			   --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/gettext
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
	)
		
	#=========================================
	# Build Remiss
	  ExternalProject_Add(
		  Remiss
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Remiss/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/remiss.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Remiss/src/remiss
		  CONFIGURE_COMMAND 
		  CMAKE_COMMAND cmake -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		  BUILD_COMMAND  $(MAKE)
		  INSTALL_COMMAND  mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Remiss && cp ${CMAKE_CURRENT_BINARY_DIR}/deps/Remiss/src/Remiss-build/libremiss.so ${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Remiss
	)
	
	#=========================================
	# Build glib
	
	# If, in building glib, an error about automake not being available happens, run: 
	#    find -type f -name Makefile.in -exec touch {} \; 
	  ExternalProject_Add(
		  Glib
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Glib/
		  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/glib-2.37.3.tar.gz"
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Glib/src/glib-2.37.3/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Glib/src/glib-2.37.3/configure --disable-static --enable-shared --enable-man=no 
			  ${xCONFIG_SDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}
			  --cache-file=${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/glibOnAndroid.cache --with-libiconv=gnu
			 --disable-xattr --disable-dtrace --disable-regex --disable-selinux 
		      "CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/gettext/include -I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/iconv/include"
		      "LDFLAGS=-lgnustl_shared -L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/iconv/lib -L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/gettext/lib"
		      "LIBS=-l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FFI/lib/libffi.a  -l${CMAKE_CURRENT_BINARY_DIR}/builds/libs/gettext/lib/libintl.a"
		      LIBFFI_CFLAGS=-I${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FFI/lib/libffi-3.0.13/include 
		      LIBFFI_LIBS=-L${CMAKE_CURRENT_BINARY_DIR}/builds/libs/FFI/lib/
		      --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Glib
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
		  DEPENDS iconv FFI gettext Remiss
	)

	#=========================================
	# Build Cairo
	# I manually changed android/deps/Cairo/src/cairo/src/cairo-output-stream.c:318
	# and android/deps/Cairo/src/cairo/src/cairo-cff-subset.c:317
	# and android/deps/Cairo/src/cairo/src/cairo-cff-subset.c:309 to
	#     decimal_point = ".";
	  ExternalProject_Add(
		  Cairo
		  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Cairo/
		  DOWNLOAD_COMMAND git clone git://anongit.freedesktop.org/git/cairo
		  
		  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Cairo/src/cairo/
		  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Cairo/src/cairo/autogen.sh ${xCONFIG_SDL} ${xCONFIG_CAIRO} 
			   -disable-trace --disable-full-testing
		       CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}
		       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Cairo
		  BUILD_COMMAND  $(MAKE) -j4
		  INSTALL_COMMAND  $(MAKE) install
		  BUILD_IN_SOURCE 0
		  DEPENDS Glib Pixman Remiss
	)
	
	#=========================================
	# Build Pango
	ExternalProject_Add(
	  PANGO
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/PANGO/
	  #DOWNLOAD_COMMAND git clone git://git.gnome.org/pango
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/pango-1.30.0.tar.gz"
	  
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/PANGO/src/pango/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/PANGO/src/pango/configure ${xCONFIG_SDL}  ${xCONFIG_PANGO} 
	       --disable-gtk-doc --enable-gtk-doc-html  --enable-shared 
	       CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}  
	       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/PANGO/pango
	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	  DEPENDS Glib Cairo
	)

endif()

if (${MAKE_GMP} STREQUAL "ON")
	#=========================================
	# Build GMP
	ExternalProject_Add(
	  GMP_BigInts
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/${GMP_Version}.tar.bz2"
	  
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/src/${GMP_Version}/
	  PATCH_COMMAND patch -p0 -d ${CMAKE_CURRENT_BINARY_DIR} -i ${CMAKE_CURRENT_SOURCE_DIR}/GMP_patch
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/GMP_BigInts/src/${GMP_Version}/configure ${xCONFIG_GMP} 
	     --enable-cxx CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS}  
	     --enable-static --disable-shared --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/GMP_BigInts/${GMP_Version}/
	  BUILD_COMMAND  $(MAKE) -j4
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
	  #CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/MPFA_Floats/src/mpfr-3.1.0/configure  CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} -fPIC ${xCONFIG_VARS}  --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/MPFA_Floats/mpfr-3.1.0/ --with-gmp=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/GMP_BigInts/${GMP_Version}/

	  #BUILD_COMMAND  $(MAKE) -j4
	  #INSTALL_COMMAND  $(MAKE) install
	  #BUILD_IN_SOURCE 0
	  #DEPENDS "GMP_BigInts"
	#)
endif()


if (${MAKE_SQL} STREQUAL "ON")
	#=========================================
	# Build sqlite3
	ExternalProject_Add(
	  sqlite3DB
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/sqlite-autoconf-3071602.tar.gz"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/src/sqlite-autoconf-3071602/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/sqlite3/src/sqlite-autoconf-3071602/configure ${xCONFIG_SQL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-static --enable-shared --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/sqlite3/sqlite-autoconf-3071602/

	  BUILD_COMMAND  $(MAKE)
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)
endif()


if (${MAKE_RDL} STREQUAL "ON")
	#=========================================
	# Build Readline
	ExternalProject_Add(
	  Readline
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/readline-6.2.tar.gz"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/src/readline-6.2/
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/Readline/src/readline-6.2/configure ${xCONFIG_RDL} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${xCONFIG_VARS} --enable-static --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/Readline/readline-6.2/

	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	  BUILD_IN_SOURCE 0
	)
endif()


if (${MAKE_GIT} STREQUAL "ON")
	#=========================================
	# Build libgit2
	ExternalProject_Add(
	  GIT_LIB
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps/GIT_LIB/
	  URL "${CMAKE_CURRENT_SOURCE_DIR}/deps/archive/libgit2-0.18.0.tar.gz"

	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/GIT_LIB/src/libgit2-0.18.0/
	  CMAKE_COMMAND cmake  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DBUILD_SHARED_LIBS=OFF -DBUILD_CLAR=OFF ${xCONFIG_VARS} -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/GIT_LIB/libgit2-0.18.0/

	  BUILD_COMMAND  $(MAKE) -j4
	  INSTALL_COMMAND  $(MAKE) install
	)
endif()


if (${MAKE_ICU} STREQUAL "ON")
	#=========================================
	# Build ICU (unicode)
	find_package(Subversion)
	ExternalProject_Add(
	  ICU_unicode_local
	  SVN_REPOSITORY "http://source.icu-project.org/repos/icu/icu/tags/${ICU_UnicodeVersion}"
	  UPDATE_COMMAND ""
	  # TODO: patch uversion.h and platform.h as per readme.html
	  PATCH_COMMAND
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/${ICU_DIR}/${ICU_UnicodeVersion}
	  
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/${ICU_DIR}/${ICU_UnicodeVersion}/source/runConfigureICU ${ICU_PLAT_CODE_local} 
	       CXX=${LOCAL_CXX} CC=${LOCAL_CC} --with-data-packaging=archive ${xCONFIG_ICU} 
	       --enable-static  --disable-shared --enable-extras=no --enable-strict=no 
	       --enable-tests=no --enable-samples=no --enable-dyload=no --enable-tools=no 
	       --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/${ICU_DIR}/${ICU_UnicodeVersion}

	  BUILD_COMMAND $(MAKE) -j4
	  INSTALL_COMMAND $(MAKE) install
	  BUILD_IN_SOURCE 1
	)
	
	if (${CROSS_COMPILE} STREQUAL 'yes')
	#=========================================
	# Build ICU (unicode) via cross compiler
	ExternalProject_Add(
	  ICU_unicode_cross
	  SVN_REPOSITORY "http://source.icu-project.org/repos/icu/icu/tags/${ICU_UnicodeVersion}"
	  UPDATE_COMMAND ""
	  # TODO: patch uversion.h and platform.h as per readme.html
	  PATCH_COMMAND
	  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/deps/unicode/${ICU_UnicodeVersion}
	  
	  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/deps/unicode/${ICU_UnicodeVersion}/source/runConfigureICU ${ICU_PLAT_CODE} 
	      ${xCONFIG_VARS} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} --with-data-packaging=archive ${xCONFIG_ICU} 
	      --enable-static --disable-shared --enable-extras=no --enable-strict=no 
	      --enable-tests=no --enable-samples=no --enable-dyload=no --enable-tools=no 
	      --prefix=${CMAKE_CURRENT_BINARY_DIR}/builds/libs/unicode/${ICU_UnicodeVersion}

	  BUILD_COMMAND $(MAKE) -j4
	  INSTALL_COMMAND $(MAKE) install
	  BUILD_IN_SOURCE 0
	  DEPENDS "ICU_unicode_local"
	)
	endif()
endif()

	add_subdirectory (core)
if (${MAKE_PRG} STREQUAL "ON")
	add_subdirectory (slip)
endif()
