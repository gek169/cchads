#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
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

void clampf(double* c, double minval, double maxval){
	if(*c < minval) *c = minval;
	else if (*c > maxval) *c = maxval;
}

//usage ./noise x1 y1 x2 y2 w h seed z
int main(int argc, char**argv){
	if(argc < 7){
		printf("usage: ./noise x1 y1 x2 y2 w h\n");exit(0);
	}
	
	double x1 = atof(argv[1]);
	double y1 = atof(argv[2]);
	double x2 = atof(argv[3]);
	double y2 = atof(argv[4]);
	uint w = atoi(argv[5]);
	uint h = atoi(argv[6]);
	srand(time(NULL));
	int seed = rand();
	if(argc > 7) seed = atoi(argv[7]);
	double z = 0;
	if(argc > 8) z = atof(argv[8]);
	if(w > 16383) w = 16383;
	if(h > 16383) h = 16383;
	char* data = (char*)malloc((size_t)w * (size_t)h * (size_t)OUTPUTCOMPONENTS);if(!data)return 0;
	for(uint i = 0; i < w * h; i++){
		double x = (double)(i%w);
		double y = (double)(i/w);
		x /= w;
		y /= h;
		//x and y are now the mix ratio between x1-x2 and y1-y2 respectively.
		double xs = x1 * x + x2 * (1.0-x);
		double ys = y1 * y + y2 * (1.0-y);
		double val = stb_perlin_noise3_seed(xs,ys,z, 0,0,0, seed);
		val += 1; val /= 2;
		val *= 255.9;
		clampf(&val, 0, 255);
		char writeme = (char)val;
		data[i*OUTPUTCOMPONENTS] = writeme;
		data[i*OUTPUTCOMPONENTS + 1] = writeme;
		data[i*OUTPUTCOMPONENTS + 2] = writeme;
	}
	stbi_write_png("out.png", w, h, OUTPUTCOMPONENTS, data, 0);
	//if(avgs) free(avgs);
	free(data);
	return 0;
}
