#UNCOMMENT_TO_DO_GCOVERAGE = -fprofile-arcs -ftest-coverage

INCLUDE = -I/usr/X11R6/include/ -I. -I/usr/include/SDL -I/usr/include/cairo -I/usr/include/freetype2
LIBDIR  = -L/usr/X11R6/lib -L../libfixmath/libfixmath

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)
PANGO_CFLAGS := $(shell pkg-config --cflags pangocairo)
PANGO_LDFLAGS := $(shell pkg-config --libs pangocairo)
UNICODE_LIBS := -licuio -licui18n -licutu -licuuc -licudata -licule -liculx -licutest -ldl
PROTEUS_OBJS = slip.o Proteus.o InfonIO.o Functions.o


COMPILERFLAGS = -fpermissive -Wno-write-strings -fno-for-scope -Wno-deprecated -g
CC = g++
CFLAGS = $(COMPILERFLAGS) $(UNCOMMENT_TO_DO_GCOVERAGE)

slip: $(PROTEUS_OBJS)
	g++ $(CFLAGS)  -L/usr/local/lib -L. -L../libfixmath/libfixmath  $(PROTEUS_OBJS) -L../unicode/install/lib $(UNICODE_LIBS) ${SDL_LDFLAGS} $(PANGO_LDFLAGS) -lSDL_image -lcairo -lfixmath -o slip

slip.o: slip.cpp ../core/Proteus.h
	g++ $(CFLAGS)  ${SDL_CFLAGS} $(PANGO_CFLAGS) $(INCLUDE) -c slip.cpp

Proteus.o: ../core/Proteus.h  ../core/Proteus.cpp
	g++ $(CFLAGS) -c -I../core/ -I../unicode/install/include/ ../core/Proteus.cpp

InfonIO.o: ../core/InfonIO.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/ -I../unicode/install/include/ ../core/InfonIO.cpp

Functions.o: ../core/Functions.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c -I../core/  -I../unicode/install/include/ ../core/Functions.cpp

.PHONY: clean

clean:
	@rm -f slip $(PROTEUS_OBJS)

.PHONY: distclean

distclean: clean
	@rm -rf ./*~ ./.*.swp .d

-include $(DEPFILES)
