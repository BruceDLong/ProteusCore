#UNCOMMENT_TO_DO_GCOVERAGE = -fprofile-arcs -ftest-coverage

INCLUDE = -I/usr/X11R6/include/ -I. -I/usr/include/SDL -I/usr/include/cairo -I/usr/include/freetype2
LIBDIR  = -L/usr/X11R6/lib

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)
PROTEUS_OBJS = slip.o Proteus.o InfonIO.o Functions.o sdl_common.o


COMPILERFLAGS = -fpermissive -fno-for-scope -Wno-deprecated -g
CC = g++
CFLAGS = $(COMPILERFLAGS) $(UNCOMMENT_TO_DO_GCOVERAGE)

slip: $(PROTEUS_OBJS)
	g++ $(CFLAGS)  -L/usr/local/lib -L. -lSDL_gfx $(PROTEUS_OBJS) ${SDL_LDFLAGS} -lSDL_ttf -lsprig -lcairo -o slip

slip.o: slip.cpp ../core/Proteus.h
	g++ $(CFLAGS)  ${SDL_CFLAGS} $(INCLUDE) -c slip.cpp

Proteus.o: ../core/Proteus.h  ../core/Proteus.cpp
	g++ $(CFLAGS) -c -I../core/  ../core/Proteus.cpp

InfonIO.o: ../core/InfonIO.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/  ../core/InfonIO.cpp

Functions.o: ../core/Functions.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/  ../core/Functions.cpp

sdl_common.o: sdl_common.c
	gcc  -g  ${SDL_CFLAGS} -I. -c sdl_common.c

.PHONY: clean

clean:
	@rm -f slip $(PROTEUS_OBJS)

.PHONY: distclean

distclean: clean
	@rm -rf ./*~ ./.*.swp .d

-include $(DEPFILES)
