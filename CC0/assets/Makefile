.ONESHELL:
all: pdtextures
clean:
	rm -rf pdtextures/512x512
	rm -rf pdtextures/256x256
	rm -rf pdtextures/1024x1024
pdtextures:
	mkdir pdtextures/512x512 || @echo ""
	mkdir pdtextures/256x256  || @echo ""
	mkdir pdtextures/1024x1024  || @echo ""
	@cd pdtextures && ./convert.sh && cd ..
