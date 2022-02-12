.ONESHELL:
CC=gcc
CPP=g++
CFLAGS=-I./include/
ALL_T=
UNAPPROVED_T= duhmenu cpp-shell cpp-shell-fork myLittlez80 myLittle6502
all: $(ALL_T)
	@echo "The following targets are not approved by the Makefile maintainer:"
	@echo $(UNAPPROVED_T)
	@echo "You can make them by invoking make <target> in GPL/"
clean:
	rm -f $(ALL_T) $(UNAPPROVED_T)
	
duhmenu:
	$(CPP) duhmenu.cpp -Os -o duhmenu
cpp-shell:
	$(CPP) cpp-shell.cpp -Os -o cpp-shell
cpp-shell-fork:
	$(CPP) cpp-shell-fork.cpp -Os -o cpp-shell-fork


