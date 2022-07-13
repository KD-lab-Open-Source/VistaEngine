#ifndef __SHAPE3D_H__
#define __SHAPE3D_H__

//#include "3dsftk.h"
#include "string"

struct faceS3D{
	unsigned short v1, v2, v3;
};

struct vrtxS3D{
	float x, y, z;
	float z1;
	float nx, ny, nz;
	float light;
	float sx, sy;
};

struct matS3D{
	float	xx, xy, xz,
			yx, yy, yz,
			zx, zy, zz;
	matS3D(){
		xx=0.5; xy=0; xz=0;
		yx=0; yy=0.5; yz=0;
		zx=0; zy=0; zz=0.5;
	};
};

struct vectS3D{
	float x, y, z;
	vectS3D(){
		x=y=z=0;
	};
};

#define MAX_NAME_MESH_LENGHT 20
struct meshS3D{
	unsigned short numVrtx;	/* Vertice count */
	vrtxS3D * vrtx;			/* List of vertices */
	unsigned short numFace;	/* Face count */
	faceS3D *face;			/* List of faces */
	matS3D matrix;
	vectS3D position;
	vectS3D scaling;

	short sizeX05;
	short sizeY05;
	float kScale;
	unsigned short *GB;
	unsigned short palLight[256];
	vectS3D SLight;

	//char name[MAX_NAME_MESH_LENGHT];
	string fModelName;
	void releaseBufs();
	void draw2Scr(int ViewX, int ViewY, float k_scale, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY);
	void calcNormals(void);
	bool load(const char* fname);//, int numMesh);
	void calcPositionAndLightOrto(int xVision, int yVision);
	void drawGouraund(void);
	void rotateAndScaling(int XA, int YA, int ZA, float kX, float kY, float kZ);
	void moveModel2(int x, int y);
	void shiftModelOn(int dx, int dy);
	void put2Earch(int quality=1);
	meshS3D(void){
		numFace=numVrtx=0;
		vrtx=NULL; face=NULL;
		int i;
		for(i=0; i<256; i++){
			unsigned char r, g, b;
			r=i>>3; g=i>>2; b=i>>3;
			palLight[i]=b+ (g<<5);// + (r<<11);
		}
		SLight.x= -1;
		SLight.y= -1;
		SLight.z= 1;
		float L=sqrtf(SLight.x*SLight.x + SLight.y*SLight.y + SLight.z*SLight.z);
		SLight.x/=L;
		SLight.y/=L;
		SLight.z/=L;

	}
};


extern meshS3D wrkMesh;


#endif //__SHAPE3D_H__
