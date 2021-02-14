.ONESHELL:
CC=gcc
CPP=g++
HEH=n
CFLAGS=-I./include/
CGLTARGET=none
ALL_T= bubble colored_text noise gif polish\
	 gmap mandel noisebox wall passgen sdf_tracer objparse s3lhelloworld
UNAPPROVED_T= sudoku threaddemo
SUB_APPROVED= lua
OTHER_SUB= omp
all: $(ALL_T) $(SUB_APPROVED)
	@echo "Note: The following targets are not approved by the makefile maintainer due to unusual deps:"
	@echo $(OTHER_SUB) $(UNAPPROVED_T)
	@echo "You can make each of them by doing make <target>"
clean:
	rm -f $(ALL_T) $(UNAPPROVED_T)
	rm -f *.exe
#	cd CGL && $(MAKE) clean && cd ..
	cd lua && $(MAKE) clean && cd ..
	cd OMP && $(MAKE) clean && cd ..
bubble:
	$(CC) bubble.c -o bubble
colored_text:
	$(CC) colored_text.c -o colored_text -lm
noise:
	$(CC) noise.c -o noise -lm
gif:
	$(CPP) gif.cpp -o gif -lm
polish:
	$(CC) polish.c -o polish -lm
objparse:
	$(CC) objparse.c -o objparse -lm
sudoku:
	$(CC) sudoku.c -o sudoku -lm
gmap:
	$(CC) gmap.c -o gmap -lm
mandel:
	$(CPP) mandel.cpp -o mandel -lm
noisebox:
	$(CC) noisebox.c -o noisebox -lm
wall:
	$(CC) wall.c -o wall -lm
passgen:
	$(CPP) passgen.cpp -o passgen -lm
sdf_tracer:
	$(CC) sdf_tracer.c -o sdf_tracer -lm -std=c99 -O3
s3lhelloworld:
	$(CC) s3lhelloworld.c -o s3lhelloworld -lm -std=c99 -O3
threaddemo:
	$(CC) threaddemo.c -o threaddemo -O2 -lm -lpthread 
cgl:
#	@read -p "if you want to build the SDL2 programs, enter all. Else, hit enter:" CGLTARGET
#	@echo "Selected target is:"
#	@echo $$CGLTARGET
#	cd CGL && $(MAKE) $$CGLTARGET
	cd CGL && $(MAKE)
omp:
	cd OMP && $(MAKE)
lua:
	@echo "WARNING!!! If you are building Lua on Msys2/Mingw, you will have to"
	@echo "set PLAT=mingw in MIT/lua/Makefile"
	@cd lua && $(MAKE)
