#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_PERLIN_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/stb_image_write.h"
//#include "stb_perlin.h"
#define OUTPUTCOMPONENTS 3
#define LOGO_THRESHHOLD 50
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
	if(argc < 7){
		printf("usage: ./wallpaper logo.png background.png br bg bb br2 bg2 bb2 fr fg fb fr2 fg2 fb2 \n");
		printf("The four RGB values specify the two gradients you want to use. Background and Logo respectively.\n");
		printf("logo.png will be stretched to super-impose on background.png\n");
		printf("The mix ratio between the two colors in the gradient is the average of the RGB for the background.\n");
		printf("You can add an additional argument at the end, --noinvert to disable the logo inverting the gradient before mapping.\n");
		exit(0);
	}
	int w = 0, h = 0, sc;
	int logoinverts = 1;
	int lw = 0, lh = 0;
	if(argc > 15) for(int i = 15; i < argc; i++) if(!strcmp("--noinvert",argv[i])) logoinverts = 0;
	char* logo_filename = argv[1];
	char* bg_filename = argv[2];
	/*
	char* fgcolorarg[2];
	char* bgcolorarg[2];
	fgcolorarg[0] = argv[3]; fgcolorarg[1] = argv[4];
	bgcolorarg[0] = argv[5]; bgcolorarg[1] = argv[6];
	*/
	#define fgcolorarg(x) argv[5+x]
	#define bgcolorarg(x) argv[3+x]
	uchar	bgc[6], //background color
			fgc[6]; //foreground color
	//Retrieve the logo and background.
	//puts("\nMARKER_0");
	unsigned char* logo_data = stbi_load(logo_filename, &lw, &lh, &sc, OUTPUTCOMPONENTS);
	unsigned char* bg_data = stbi_load(bg_filename, &w, &h, &sc, OUTPUTCOMPONENTS);
	printf("\nw = %d, h = %d",w,h);
	if(!logo_data || lw < 1 || lh < 1){printf("\nBAD LOGO FILE %s\nERROR!!!\n",logo_filename);exit(1);}
	if(!bg_data || w < 1 || h < 1){printf("\nBAD BACKGROUND FILE %s\nERROR!!!\n",bg_filename);exit(1);}
	bgc[0] = (uchar)atoi(argv[3]);
	bgc[1] = (uchar)atoi(argv[4]);
	bgc[2] = (uchar)atoi(argv[5]);
	bgc[3] = (uchar)atoi(argv[6]);
	bgc[4] = (uchar)atoi(argv[7]);
	bgc[5] = (uchar)atoi(argv[8]);

	fgc[0] = (uchar)atoi(argv[9]);
	fgc[1] = (uchar)atoi(argv[10]);
	fgc[2] = (uchar)atoi(argv[11]);
	fgc[3] = (uchar)atoi(argv[12]);
	fgc[4] = (uchar)atoi(argv[13]);
	fgc[5] = (uchar)atoi(argv[14]);
	//puts("\nMARKER_3");
	//int dbg = 0;
	for(size_t i = 0; i < (size_t)w * (size_t)h; i++){
		//The place to write to in the destination AND the background sample point.
		size_t x = i%w, 
			y = i/w;
		if(y >= (size_t)h || x >= (size_t)w)
		{
			printf("\nBad Array Bounds!\n");exit(1);
		}
		//Enable debug printout for this for loop.
		//if(y >= (size_t)h-1 && x >= (size_t)w-50) dbg = 1;
		double	px = (double)x/(double)w,
				py = (double)y/(double)h;
		int lx = px * lw,
			ly = py * lh;
		clampi(&lx,0,lw-1);
		clampi(&ly,0,lh-1);
		uchar bg_sample[3], lg_sample[3]; double bg_avg, logo_avg; 
		//puts("\nMARKER_4");
		bg_sample[0] = bg_data[3*i];
		bg_sample[1] = bg_data[3*i+1];
		bg_sample[2] = bg_data[3*i+2];
		//puts("\nMARKER_5");
		lg_sample[0] = logo_data[3*(lx+ly*lw)];
		lg_sample[1] = logo_data[3*(lx+ly*lw)+1];
		lg_sample[2] = logo_data[3*(lx+ly*lw)+2];
		//puts("\nMARKER_6");
		bg_avg =(double)bg_sample[0] * 1.0/3.0 * 1/255.0 + 
				(double)bg_sample[1] * 1.0/3.0 * 1/255.0 + 
				(double)bg_sample[2] * 1.0/3.0 * 1/255.0;
		logo_avg =	(double)lg_sample[0] * 1.0/3.0  + 
					(double)lg_sample[1] * 1.0/3.0  + 
					(double)lg_sample[2] * 1.0/3.0;
		uchar logo_binary = logo_avg > LOGO_THRESHHOLD?255:0; //Is this pixel inside of the logo?
		uchar* grad = logo_binary?fgc:bgc; //Pick which gradient we use
		clampf(&bg_avg, 0.0, 1.0);
		if(logoinverts && logo_binary) bg_avg = 1.0 - bg_avg;
		//if(dbg) puts("\nBEFORE");
		bg_data[i*3]   = grad[3] * bg_avg + grad[0] * (1.0-bg_avg);
		bg_data[i*3+1] = grad[4] * bg_avg + grad[1] * (1.0-bg_avg);
		bg_data[i*3+2] = grad[5] * bg_avg + grad[2] * (1.0-bg_avg);
		//if(dbg) puts("\nAFTER");
	}
	printf("\nFinished main loop!");
	stbi_write_png("wall.png", w, h, OUTPUTCOMPONENTS, bg_data, 0);
	free(logo_data);
	free(bg_data);
	//free(data);
	return 0;
}
