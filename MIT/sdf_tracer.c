#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"
#define OUTPUTCOMPONENTS 3
#define CHAD_MATH_IMPL
#include "include/3dMath.h"
//SDF TRACER DEFINITION
typedef struct{
	f_ (*SDF)(void*, vec3);//returns signed distance
	vec3 (*COL)(void*, vec3, f_);//returns color at the point, (THIS, eye, SD)
	void* subobj[2]; //Subobjects for CSG Unions and Intersections.
	f_ shapedef[32]; //Shape properties. E.g. Sphere might use Radius as the first parameter.
	vec3 pos, rot, color, color2; //Shape Data. 2 colors allows for gradient type shit to happen.
	f_ diff, specr, specd; //phong properties
} tobject;
typedef struct {
	vec3 color, pos;
	f_ radius;
} pointlight;
tobject* Objects = NULL; uint numobjects = 0;
pointlight* lights = NULL; uint numlights = 0;
//SHAPE FUNCS
f_ Sphere_SDF(void* me, vec3 eye){
	//puts("\nSphere SDF being used.");
	return	lengthv3(subv3(
				eye,
				((tobject*)me)->pos
			))
	- ((tobject*)me)->shapedef[0];
}
//COLOR FUNCS
vec3 FlatColor(void* me, vec3 eye, f_ dist){return ((tobject*)me)->color;}
//SDF TRACER MARCHING FUNCTION, returns color at pixel.
#define MAX_MARCHES 80
#define MAX_REFLECT 3
#define EPSILON 0.002
//#define BEGIN_DIST 1000 /*Essentially defines a "maximum range"*/
#define DIST_THRESHHOLD 0.001 /*Dist threshhold is how small the SDF has to return for a hit to be registered.*/
const float BOUNCEOFFSET = 1.2; /*How much is the "eye" moved forward on a reflection?*/
#define RET_BACKGROUND_COLOR vec3 BackCol; BackCol.d[0] = 0; BackCol.d[1] = 0; BackCol.d[2] = 0.3; return BackCol;
vec3 march_scene(vec3 ray, vec3 eye, uint reflect_iter, tobject* exception, f_ BEGIN_DIST){
	if(MAX_REFLECT < reflect_iter) {RET_BACKGROUND_COLOR}
	vec3 campos = eye;
	for(uint i = 0; i < MAX_MARCHES; i++){
		f_ dist = BEGIN_DIST;
		tobject* closest = NULL;
		for(uint j = 0; j < numobjects; j++) //TODO ignore exception's children...?
		{
			if(Objects+j == exception) continue;
			f_ dj = Objects[j].SDF((void*)(&(Objects[j])),eye);
			closest = (dj < dist)? &(Objects[j]) : closest;
			dist = (dj < dist)? dj : dist;
		}
		if(!closest) {RET_BACKGROUND_COLOR;} //There was nothing in range.
		if(dist < DIST_THRESHHOLD){ //Collision! 
			//Calculate normal.
			vec3 eyev[6];
			eyev[0] = eye; eyev[0].d[0] += EPSILON;
			eyev[1] = eye; eyev[1].d[0] -= EPSILON;
			eyev[2] = eye; eyev[2].d[1] += EPSILON;
			eyev[3] = eye; eyev[3].d[1] -= EPSILON;
			eyev[4] = eye; eyev[4].d[2] += EPSILON;
			eyev[5] = eye; eyev[5].d[2] -= EPSILON;
			vec3 norm = (vec3){
				.d[0] = closest->SDF((void*)closest,eyev[0]) - closest->SDF((void*)closest,eyev[1]),
				.d[1] = closest->SDF((void*)closest,eyev[2]) - closest->SDF((void*)closest,eyev[3]),
				.d[2] = closest->SDF((void*)closest,eyev[3]) - closest->SDF((void*)closest,eyev[5])
			}; norm = normalizev3(norm);
			vec3 specaccum = (vec3){.d[0] = 0, .d[1] = 0, .d[2] = 0};
			vec3 diffaccum = (vec3){.d[0] = 0, .d[1] = 0, .d[2] = 0};
			//if(numlights == 0) puts("\nNO LIGHTS!!!!");
			vec3 e2c = normalizev3(subv3(campos,eye));
			for(uint q = 0; q < numlights; q++){
				pointlight light = lights[q]; //TODO: make this more efficient later.
				vec3 e2l = subv3(light.pos,eye); //eye 2 light
				f_ d2l = lengthv3(e2l);
				vec3 unit_e2l = scalev3(1.0/d2l, e2l);
				f_ begdist = d2l;
				uint hit=0;
				vec3 eye2 = addv3(eye,scalev3(BOUNCEOFFSET,unit_e2l));
				vec3 ray2 = subv3(light.pos,eye2);
				ray2 = normalizev3(ray2);
				for(uint i = 0; i < MAX_MARCHES; i++){
					f_ dist = begdist;
					tobject* closestl = NULL;
					for(uint j = 0; j < numobjects; j++) //TODO ignore exception and its children.
					{
						//if(Objects+j == closest) continue;
						if(Objects+j == exception) continue;
						f_ dj = Objects[j].SDF((void*)(&(Objects[j])),eye2);
						closestl = (dj < dist)? &(Objects[j]) : closestl;
						dist = (dj < dist)? dj : dist;
					}
					if(!closestl) {break;} //Nothing in range.
					if(dist < DIST_THRESHHOLD){hit=1;break;}
					eye2 = addv3(eye2,scalev3(clampf(0.99*dist,0,begdist),ray2));
					vec3 test = subv3(light.pos,eye2);
					begdist = lengthv3(test);
					//if(dotv3(test,light.pos)<0){break;} //We have gone past the light.
				}//EOF marching to lights.
				if(hit)continue; //it's in shadow! Don't do anything for this light.
				//Attempt to do lighting for this light.
				f_ dropoffmult = (light.radius-d2l)/light.radius;
				dropoffmult = clampf(dropoffmult,0,1);
				diffaccum = addv3(
					diffaccum, //diffaccum + 
					scalev3(
						dropoffmult * clampf(dotv3(unit_e2l,norm),0,1), //dropoff * dotproduct of normal and e2l
						light.color//times the light's color.
						)
				);
				vec3 lightdir = scalev3(-1,unit_e2l);
				
				vec3 unit_e2c = normalizev3(e2c);
				f_ specFactor = fmax(	dotv3(reflect(lightdir,norm),unit_e2c)	,0	);
				f_ specDampFactor = pow(specFactor, closest->specd);
				
				specaccum = addv3(
					specaccum,
					//+
					clampvec3(scalev3(closest->specr * specDampFactor * dropoffmult,light.color),
						(vec3){.d[0]=0,.d[1]=0,.d[2]=0},
						(vec3){.d[0]=1,.d[1]=1,.d[2]=1}
					)
				);
				
			}//EOF light handling code.
			//Do mirror reflections.
			vec3 reflectdir = normalizev3(reflect(scalev3(-1,e2c),norm));
			vec3 normscaled = scalev3(BOUNCEOFFSET, norm);
			//normscaled = addv3(normscaled,scalev3(BOUNCEOFFSET, reflectdir));
			specaccum = addv3(specaccum,
				clampvec3(
					scalev3(closest->specr,
						march_scene(reflectdir, addv3(eye,normscaled),reflect_iter+1,NULL,BEGIN_DIST)
					),
					(vec3){.d[0]=0,.d[1]=0,.d[2]=0},(vec3){.d[0]=1,.d[1]=1,.d[2]=1}
				)
			);
			//return closest->COL((void*)closest, eye, dist);
			diffaccum = clampvec3(diffaccum,
						(vec3){.d[0]=0,.d[1]=0,.d[2]=0},
						(vec3){.d[0]=1,.d[1]=1,.d[2]=1});
			specaccum = clampvec3(specaccum,
						(vec3){.d[0]=0,.d[1]=0,.d[2]=0},
						(vec3){.d[0]=1,.d[1]=1,.d[2]=1});
			return clampvec3(
								addv3(scalev3(closest->diff,multvec3(closest->color,diffaccum)),specaccum),
								(vec3){.d[0]=0,.d[1]=0,.d[2]=0},
								(vec3){.d[0]=1,.d[1]=1,.d[2]=1}
							);
		}
		dist = clampf(dist, 0, BEGIN_DIST); //Ensure that dist is not less than zero. Shouldn't be necessary.
		eye = addv3(eye,scalev3(0.99*dist,ray)); //March onward!
	}
	RET_BACKGROUND_COLOR;
}

  vec3 mincol(){vec3 ret; ret.d[0] = 0; ret.d[1] = 0; ret.d[2] = 0;return ret;}
  vec3 maxcol(){vec3 ret; ret.d[0] = 255; ret.d[1] = 255; ret.d[2] = 255;return ret;}
int main(int argc, char** argv){
	uint width = 640, height = 480; f_ ratio = (f_)width / height;
	f_ screenHeight = 8.0;
	f_ screenWidth = ratio * screenHeight;
	vec3 topleft;
	topleft.d[0] = ratio * (-screenHeight/2.0);
	topleft.d[1] = screenHeight/2;
	topleft.d[2] = -5.0;
	srand(time(NULL));
	numobjects = rand()%10 + 20;
	Objects = (tobject*)malloc(sizeof(tobject) * numobjects);
	numlights = 3;
	lights = (pointlight*)malloc(sizeof(pointlight)*numlights);
	
	lights[0].color = (vec3){.d[0]=1.0, .d[1]=1.0, .d[2]=1.0};
	lights[0].pos = (vec3){.d[0]=-19.0, .d[1]=14.0, .d[2]=-6.0};
	lights[0].radius = 100.0;
	lights[1].color = (vec3){.d[0]=0.15, .d[1]=0.8, .d[2]=0.8};
	lights[1].pos = (vec3){.d[0]=10.0, .d[1]=-2.0, .d[2]=-5.0};
	lights[1].radius = 400.0;
	lights[2].color = (vec3){.d[0]=1.0, .d[1]=1.0, .d[2]=1.0};
	lights[2].pos = (vec3){.d[0]=-1.0, .d[1]=-14.0, .d[2]=-8.0};
	lights[2].radius = 100.0;
	Objects[0].SDF = Sphere_SDF; //Sphere shape
	Objects[0].COL = FlatColor; //Flat colored
	Objects[0].subobj[0] = NULL;
	Objects[0].subobj[1] = NULL;
	Objects[0].shapedef[0] = 5; //radius 5 sphere
	Objects[0].color.d[0] = 1.0;
	Objects[0].specr = 0.2;
	Objects[0].specd = 30;
	Objects[0].color.d[1] = 0.0;
	Objects[0].color.d[2] = 0.0;
	Objects[0].pos.d[0] = 0.0;
	Objects[0].pos.d[1] = 3.0;
	Objects[0].pos.d[2] = 10.0; //far away from the eye!
	Objects[0].diff = 1.0; //far away from the eye!
	for(size_t i = 1; i < numobjects; i++){
		Objects[i] = Objects[0]; //base configuration.
		Objects[i].pos.d[0] = (f_)rand()/RAND_MAX * 100 - 50;
		Objects[i].pos.d[1] = (f_)rand()/RAND_MAX * 100 - 50;
		Objects[i].pos.d[2] = (f_)rand()/RAND_MAX * 100 - 5;
		Objects[i].color.d[0] = (f_)rand()/RAND_MAX;
		Objects[i].color.d[1] = (f_)rand()/RAND_MAX;
		Objects[i].color.d[2] = (f_)rand()/RAND_MAX;
		Objects[i].specr = (f_)rand()/RAND_MAX;
		Objects[i].diff = 1 - Objects[i].specr;
		Objects[i].specd = (f_)rand()/RAND_MAX*40+10;
		Objects[i].shapedef[0] = (f_)rand()/RAND_MAX*10+3;
		if(i==1){
			Objects[i].pos.d[0] = Objects[0].pos.d[0]+5;
			Objects[i].pos.d[1] = Objects[0].pos.d[1]+3;
			Objects[i].pos.d[2] = Objects[0].pos.d[2]-2.9;
			Objects[i].shapedef[0] = 3;
		}
		
	}
	unsigned char* data = malloc(width * height * 3);
	for(uint x = 0; x < width; x++)
	for(uint y = 0; y < height; y++)
	{
		vec3 eye, ray, screen;
		eye.d[0] = 0; eye.d[1] = 0; eye.d[2] = -10; //screen starts at -5
		screen = topleft;
		screen.d[0] += (f_)x/width * screenWidth;
		screen.d[1] -= (f_)y/height * screenHeight; // Negative Y is down.
		ray = normalizev3(subv3(screen,eye));
		vec3 col = march_scene(ray, eye, 0, NULL, 1000);
		col = clampvec3(scalev3(256,col),mincol(),maxcol());
		data[(x+y*width)*3] = col.d[0];
		data[(x+y*width)*3+1] = col.d[1];
		data[(x+y*width)*3+2] = col.d[2];
	}
	stbi_write_png("trace.png", width, height, 3, data, 0);
	if(data)free(data);
	if(lights)free(lights);
	if(Objects)free(Objects);
	return 0;
}