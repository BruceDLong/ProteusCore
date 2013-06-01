#!/bin/sh
echo "RUNNING IMAGE CONFIG SCRIPT"
bld=${1}
bld="${bld%\"}"
bld="${bld#\"}"

args=${2}
args="${args%\"}"
args="${args#\"}"
echo "SDL_IMAGE Build folder: ${bld}"
echo "Config arguments: ${args}"

export SDL_CONFIG=${bld}/builds/libs/SDL/1.3/bin/sdl2-config
cd ${bld}/deps/SDL_image/src/SDL_image/
${bld}/deps/SDL_image/src/SDL_image/autogen.sh
${bld}/deps/SDL_image/src/SDL_image/configure ${args} --prefix=${bld}/builds/libs/SDL_image
