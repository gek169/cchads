.ONESHELL:
ALL_T=
UNAPPROVED_T= wall.png
all: 
	@echo "The following targets are not approved by the makefile maintainer:"
	@echo $(UNAPPROVED_T)
	@echo "You can create them by calling make <target> in LGPL/"
clean:
	rm -f wall.png out.png
wall.png:
	sh ./make_thing.sh