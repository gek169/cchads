#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_PERLIN_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/stb_image_write.h"
#include "include/stb_perlin.h"
#define OUTPUTCOMPONENTS 3
//gcc -lm GradientMap.c -o GradientMap
//Performs a similar function to the "Gradient Map" tool in Gimp
//Licensed to you under the CC0 license. https://creativecommons.org/publicdomain/zero/1.0/legalcode

typedef unsigned int uint;
typedef unsigned char uchar;
void clampf(double* c, double minval, double maxval){
	if(*c < minval) *c = minval;
	else if (*c > maxval) *c = maxval;
}
void clampi(int* c, int minval, int maxval){
	if(*c < minval) *c = minval;
	else if (*c > maxval) *c = maxval;
}
double c2d(uchar in){return in/255.0;}
uchar d2c(double in){return in*255.0;}
int main(int argc, char**argv){
	if(argc < 8){
		printf("usage: ./noisebox w h fmult ibias x1 y1 x2 y2 z seed [OPERATOR] fmult ibias x1 y1 x2 y2 z seed [OPERATOR] ...\n");
		printf("You can generate a single layer of noise, or up to 1024 with blending between.\n");
		printf("x2 and y2 are offsets from x1 and y1.\n");
		printf("Operators: AND OR NAND NOR XOR MULT DIV ADD SUB OVERADD OVERSUB\n");
		printf("AND, OR, NAND, NOR, XOR are bitwise ops\n");
		printf("OVERMULT MULT OVERDIV DIV are floating point on the 0..1 range of the number.\n");
		printf("ADD and SUB use integer addition and subtraction. OVERADD and OVERSUB allow for integer over/underflow\n");
		printf("FMULT is applied before IBIAS\n");
		printf("All blending is performed after fmult and fbias are applied.\n");
		printf("Blending is applied bottom up. The first entry will be written, then blended by later operators.\n");
		printf("Example: ./noisebox 640 480 2.0 25 0.0 0.0 6.4 4.8 0.0 200 AND 1.0 0 10.0 10.0 20.0 20.0 8.8 130\n");
		exit(0);
	}
	uint w = atoi(argv[1]);
	uint h = atoi(argv[2]);
	if(w > 16383) w = 16383;
	if(h > 16383) h = 16383;
	double x1[1024], y1[1024], x2[1024], y2[1024], z[1024], sample[1024];
	double fmults[1024];
	int ibiases[1024]; 
	uchar operators[1024];uchar csample[1024];
	int seed[1024];
	int commandpart = 0;
	int command_number = 0;
	int completed_commands = 0;
	for(int i = 3; i < argc; i++){
		if(command_number > 1023) break;
		switch(commandpart){
			default:
				//printf("\nHandling operator...\n");
				if(!strcmp(argv[i],"AND")){
					operators[command_number] = '&';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"OR")){
					operators[command_number] = '|';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"NAND")){
					operators[command_number] = 'N';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"NOR")){
					operators[command_number] = 'n';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"XOR")){
					operators[command_number] = '^';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"MULT")){
					operators[command_number] = '*';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"DIV")){
					operators[command_number] = '/';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"OVERMULT")){
					operators[command_number] = 'M';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"OVERDIV")){
					operators[command_number] = 'D';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"ADD")){
					operators[command_number] = '+';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"SUB")){
					operators[command_number] = '-';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"OVERADD")){
					operators[command_number] = 'a';command_number++;commandpart = -1;break;
				}
				if(!strcmp(argv[i],"OVERSUB")){
					operators[command_number] = 's';command_number++;commandpart = -1;break;
				}
				printf("\nMissing or invalid operator!!!\n");exit(1);
			case 0: fmults[command_number] = atof(argv[i]); /*printf("\nfmults[%d] = %f\n",command_number,fmults[command_number]);*/break;
			case 1: ibiases[command_number] = atoi(argv[i]); break;
			case 2: x1[command_number] = atof(argv[i]); break;
			case 3: y1[command_number] = atof(argv[i]); break;
			case 4: x2[command_number] = atof(argv[i]) + x1[command_number]; break;
			case 5: y2[command_number] = atof(argv[i]) + y1[command_number]; break;
			case 6: z[command_number] = atof(argv[i]); break;
			case 7: seed[command_number] = atoi(argv[i]); completed_commands++;break;
		}
		commandpart++;
	}
	printf("\n--Recieved %d complete layer commands--\n", completed_commands);
	uchar* data = malloc(w * h * OUTPUTCOMPONENTS);
	for(size_t i = 0; i < w * h; i++){
		double x = (i%w)/(double)w;
		double y = (i/w)/(double)h;
		for(int n = 0; n < completed_commands && n<1024; n++){
			double	xs = x * x2[n] + (1-x) * x1[n], 
					ys = y * y2[n] + (1-y) * y1[n];
			sample[n] = stb_perlin_noise3_seed(xs,ys,z[n], 0,0,0, seed[n]);
			sample[n] += 1.0;
			sample[n] /= 2.0;
			sample[n] *= fmults[n];
			sample[n] += ibiases[n]/255.0;
			clampf(sample+n, 0.0, 1.0);
			csample[n] = d2c(sample[n]);
		} //We now have all the samples. Time to perform blending.
		uchar* target = data + i * OUTPUTCOMPONENTS;
		uchar outval = csample[0];
		int working_int;
		double working_double;
		for(int i = 1; i < completed_commands;i++){
			uchar op = operators[i-1];
			uchar arg = csample[i];
			switch(op){
				case '&': outval &= arg; break;
				case '|': outval |= arg; break;
				case '^': outval ^= arg; break;
				case 'N': outval = ~(outval & arg); break;//Nand
				case 'n': outval = ~(outval | arg); break;//Nor
				case '+': working_int = (int)outval + (int)arg; clampi(&working_int, 0,255); outval = working_int;break;
				case '-': working_int = (int)outval - (int)arg; clampi(&working_int, 0,255); outval = working_int;break;
				case 'a': outval += arg;break;
				case 's': outval -= arg;break;
				case '*': working_double = c2d(outval) * c2d(arg);clampf(&working_double, 0,1); outval = d2c(working_double);break;
				case 'M': working_double = c2d(outval) * c2d(arg);outval = d2c(working_double);break;
				case '/': working_double = c2d(outval) / c2d(arg);clampf(&working_double, 0,1); outval = d2c(working_double);break;
				case 'D': working_double = c2d(outval) / c2d(arg);outval = d2c(working_double);break;
			}
		}
		target[0] = outval;
		target[1] = outval;
		target[2] = outval;
	}
	stbi_write_png("out.png", w, h, OUTPUTCOMPONENTS, data, 0);
	//if(avgs) free(avgs);
	free(data);
	return 0;
}
