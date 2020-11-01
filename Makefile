all: mit gpl lgpl bsd3 cc0
clean:
	@cd MIT && $(MAKE) clean
	@cd GPL && $(MAKE) clean
	@cd LGPL && $(MAKE) clean
	@cd BSD3 && $(MAKE) clean
	@cd CC0 && $(MAKE) clean
cc0:
	@cd CC0 && $(MAKE)
mit:
	@cd MIT && $(MAKE)
gpl:
	@cd GPL && $(MAKE)
lgpl:
	@cd LGPL && $(MAKE)
bsd3:
	@cd BSD3 && $(MAKE)
