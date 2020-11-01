#include "include/tobjparse.h"
#include <stdio.h>
#include <math.h>

int main(int argc, char** argv){
	if(argc < 2){
		puts("Usage:\n%s <filename>");
		exit(1);
	}
	objraw omodel;
	omodel = tobj_load(argv[1]);
	// if(omodel.positions)
		// printf("\nHas %d positions",sb_count(omodel.positions));
	// if(omodel.texcoords)
		// printf("\nHas %d texcoords",sb_count(omodel.texcoords));
	// if(omodel.normals)
		// printf("\nHas %d normals",sb_count(omodel.normals));
	// if(omodel.colors)
		// printf("\nHas %d colors",sb_count(omodel.colors));
	// if(omodel.faces)
		// printf("\nHas %d faces, or %d indices",sb_count(omodel.faces)/3,sb_count(omodel.faces));
	model m = tobj_tomodel(&omodel);
	printf("\nExited that routine!");
	if(m.d)
		for(int i = 0; i < m.npoints;i++){
			if(i%3 == 0)printf("\n[ Triangle %d ]",i/3);
			printf("\nv: %f %f %f",
				m.d[i].d[0],
				m.d[i].d[1],
				m.d[i].d[2]
			);
		}
	freeobjraw(&omodel);
	freemodel(&m);
}