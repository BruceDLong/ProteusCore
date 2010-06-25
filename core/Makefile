#UNCOMMENT_TO_DO_GCOVERAGE = -fprofile-arcs -ftest-coverage

INCLUDE = -I/usr/X11R6/include/ 
LIBDIR  = -L/usr/X11R6/lib

COMPILERFLAGS = -fpermissive -Wno-deprecated -g
CC = g++
CFLAGS = $(COMPILERFLAGS) $(UNCOMMENT_TO_DO_GCOVERAGE)
#LIBRARIES = -lX11 -lXi ./libopenglut.a -lGL -lGLU -lm #./libopenglut_la-og_stroke_mono_roman.o ./libopenglut_la-og_stroke_roman.o
	
proteus: ptest.o Proteus.o InfonIO.o
	g++ $(CFLAGS) ptest.o ./Proteus.o InfonIO.o -o proteus 

ptest.o: ptest.cpp ../core/Proteus.h
	g++ $(CFLAGS) -c ptest.cpp
	
Proteus.o: ../core/Proteus.h  ../core/Proteus.cpp
	g++ $(UNCOMMENT_TO_DO_GCOVERAGE) -fpermissive -g -c -I../core/  ../core/Proteus.cpp 
	
InfonIO.o: ../core/InfonIO.cpp ../core/Proteus.h
	g++ $(UNCOMMENT_TO_DO_GCOVERAGE) -fpermissive -g -c -I../core/  ../core/InfonIO.cpp
