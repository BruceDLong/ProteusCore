#!/bin/sh
echo "RUNNING IMAGE CONFIG SCRIPT"
export SDL_CONFIG=${1}/builds/libs/SDL/1.3/bin/sdl2-config
cd ${1}/deps/SDL_image/src/SDL_image/
${1}/deps/SDL_image/src/SDL_image/autogen.sh
${1}/deps/SDL_image/src/SDL_image/configure ABI=32  CFLAGS=-m32 CXXFLAGS=-m32 LDFLAGS=-m32  --prefix=${1}/builds/libs/SDL_image
