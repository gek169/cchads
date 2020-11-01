#define LOCKSTEPTHREAD_IMPL
#include "include/lockstepthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//LockStep Thread Demo
//Useful for game engines and other situations
//Where you want threads to work in "lock step"
unsigned char a1, a2;
void JobThread1(){
	unsigned char copy = a1;
	copy++;
	a1 = copy;
}
void JobThread2(){
	unsigned char copy = a2;
	copy--;
	a2 = copy;
}
int main(){
	char inputline[2048];
	inputline[2047] = '\0';
	lsthread t1, t2;
	init_lsthread(&t1);
	init_lsthread(&t2);
	t1.execute = JobThread1;
	t2.execute = JobThread2;
	printf(
	"\nThis program demonstrates threading by having"
	"\nTwo threads \"walk\" toward each other using unsigned chars."
	"\nunsigned Integer overflow guarantees the two will converge."
	);
	printf("\nEnter a number for thread 1 to process: ");
	fgets(inputline, 2047,stdin);
	a1 = (unsigned char)atoi(inputline);
	printf("\nEnter a number for thread 2 to process: ");
	fgets(inputline, 2047,stdin);
	a2 = (unsigned char)atoi(inputline);
	unsigned int i = 0;
	//These threads start locked
	start_lsthread(&t1);
	start_lsthread(&t2);
	puts("\nGot past starts!");
	do{
		lock(&t1);
		lock(&t2);
		printf("\n%u: a1 = %d, a2 = %d",i++,(int)a1,(int)a2);
		step(&t1);
		step(&t2);
	}while(a1 < a2);
	fflush(stdout);
	kill_lsthread(&t2);
	kill_lsthread(&t1);
	printf("\nFINAL STATE! %u: a1 = %d, a2 = %d",i++,(int)a1,(int)a2);
	destroy_lsthread(&t1);
	destroy_lsthread(&t2);
	return 0;
}