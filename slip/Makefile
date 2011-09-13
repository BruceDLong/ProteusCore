#UNCOMMENT_TO_DO_GCOVERAGE = -fprofile-arcs -ftest-coverage

INCLUDE = -I/usr/X11R6/include/  -I. -I/usr/include/SDL
LIBDIR  = -L/usr/X11R6/lib

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

COMPILERFLAGS = -fpermissive -Wno-deprecated -g
CC = g++
CFLAGS = $(COMPILERFLAGS) $(UNCOMMENT_TO_DO_GCOVERAGE)

slip: slip.o Proteus.o InfonIO.o Functions.o fastevents.o
	g++ $(CFLAGS)  -L/usr/local/lib -lSDL_gfx slip.o fastevents.o ./Proteus.o InfonIO.o Functions.o ${SDL_LDFLAGS} -lSDL_ttf  -o slip

slip.o: slip.cpp ../core/Proteus.h
	g++ $(CFLAGS)  ${SDL_CFLAGS} -I. -c slip.cpp

Proteus.o: ../core/Proteus.h  ../core/Proteus.cpp
	g++ $(CFLAGS) -c -I../core/  ../core/Proteus.cpp

InfonIO.o: ../core/InfonIO.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/  ../core/InfonIO.cpp

Functions.o: ../core/Functions.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/  ../core/Functions.cpp

fastevents.o: fastevents.c
	g++ -g ${SDL_CFLAGS} -I. -I/usr/include/SDL -c fastevents.c

.PHONY: clean

clean:
	@rm -f slip $(PROTEUS_OBJS)

.PHONY: distclean

distclean: clean
	@rm -rf ./*~ ./.*.swp .d

-include $(DEPFILES)
