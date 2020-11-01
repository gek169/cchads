all: 
	@echo "Nothing to build"
	@echo "NOTE: Makefile maintainer has not approved target ok to be added to makefile tree"
	@echo "To build it, type \"make ok\""
clean: 
	rm -f ok
ok:
	gcc -static ok.c /usr/local/plan9/lib/lib9.a -o ok -O2 -I${HOME}/9base/lib9
