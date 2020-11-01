#include <stdlib.h>
#include <stdio.h>
int main (int argc, char** argv) {
	if(argc < 2)
		printf("\033[1;31m");
	else
		printf("\033%s",argv[1]);
	printf("Hello world\n");
	printf("\033[0m;");
	return 0;
}