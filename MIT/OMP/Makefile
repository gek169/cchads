.ONESHELL:
CC=gcc
CPP=g++
HEH=n
CFLAGS=-I./include/
CGLTARGET=none
ALL_T= helloworld
all: $(ALL_T)
clean:
	rm -f $(ALL_T)
	rm -f *.exe
helloworld:
	gcc helloworld.c -o helloworld -fopenmp -lgomp -lpthread -lm -std=c99 -O3