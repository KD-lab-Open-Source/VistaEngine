#include "StdAfxRD.h"
#include "Perlin.h"
#include "XMath\fastMath.h"
#include "Render\3dx\UMath.h"

inline int remainder(float x,int dx,float& cx)//x - число, dx - цикл, return - целая часть цикла, cx - дробная часть
{
	float f=fmodFast(x,(float)dx);
	if(f<0)
		f+=dx;
	int i=(int)f;

	if(i>=dx)
	{
		cx=0;
		return 0;
	}

	cx=f-i;
	return i;
}

inline
float trilinear(const float p000, const float p001,
				const float p010, const float p011,
				const float p100, const float p101, 
				const float p110, const float p111, 
				float cx,float cy, float cz)
{
	float p00=p000*(1-cx)+p001*cx;
	float p01=p010*(1-cx)+p011*cx;
	float p10=p100*(1-cx)+p101*cx;
	float p11=p110*(1-cx)+p111*cx;

	float p0=p00*(1-cy)+p01*cy;
	float p1=p10*(1-cy)+p11*cy;
	float p=p0*(1-cz)+p1*cz;
	return p;
}

inline
float tricos(const float p000, const float p001,
				const float p010, const float p011,
				const float p100, const float p101, 
				const float p110, const float p111, 
				float cx,float cy, float cz)
{
	float fx=(1-cosf(cx*3.14159f))*0.5f;
	float fy=(1-cosf(cy*3.14159f))*0.5f;
	float fz=(1-cosf(cz*3.14159f))*0.5f;

	float p00=p000*(1-fx)+p001*fx;
	float p01=p010*(1-fx)+p011*fx;
	float p10=p100*(1-fx)+p101*fx;
	float p11=p110*(1-fx)+p111*fx;

	float p0=p00*(1-fy)+p01*fy;
	float p1=p10*(1-fy)+p11*fy;
	float p=p0*(1-fz)+p1*fz;
	return p;
}

Perlin3d::Perlin3d(int dx_,int dy_,int dz_)
{
	dx=dx_;
	dy=dy_;
	dz=dz_;
	int size=dx*dy*dz;
	data=new float[size];
	for(int i=0;i<size;i++)
		data[i]=xm_random_generator.frand();
}

Perlin3d::~Perlin3d()
{
	delete[] data;
}

void Perlin3d::setNumOctave(int num)
{
	int old_num=mul_octave.size();
	mul_octave.resize(num);

	float pow=1.0f/(1<<old_num);
	for(int i=old_num;i<num;i++)
	{
		mul_octave[i]=pow;
		pow*=0.5f;
	}
}

void Perlin3d::setOctave(int i,float mul)
{
	xassert(i>=0 && i<mul_octave.size());
	mul_octave[i]=mul;
}

float Perlin3d::get(float x,float y,float z)
{
	float sum=0;
	for(int i=0;i<mul_octave.size();i++)
	{
		sum+=interpolate(x,y,z)*mul_octave[i];
		x*=2;y*=2;z*=2;
	}
	return sum;
}


float Perlin3d::get_data(int x,int y,int z)
{
	xassert(x>=0 && x<dx);
	xassert(y>=0 && y<dy);
	xassert(z>=0 && z<dz);
	return data[x+y*dx+z*dx*dy];
}

float Perlin3d::interpolate(float x,float y,float z)
{
	float cx,cy,cz;
	int ix=remainder(x,dx,cx);
	int iy=remainder(y,dy,cy);
	int iz=remainder(z,dz,cz);
	int ix1=(ix+1)%dx;
	int iy1=(iy+1)%dy;
	int iz1=(iz+1)%dz;

	float p000=get_data(ix,iy,iz);
	float p001=get_data(ix1,iy,iz);
	float p010=get_data(ix,iy1,iz);
	float p011=get_data(ix1,iy1,iz);
	float p100=get_data(ix,iy,iz1);
	float p101=get_data(ix1,iy,iz1);
	float p110=get_data(ix,iy1,iz1);
	float p111=get_data(ix1,iy1,iz1);

	return trilinear(p000,p001,
				p010,p011,
				p100,p101, 
				p110,p111, 
				cx,cy,cz);
}

///////////////////////////////////Perlin2d/////////
Perlin2d::Perlin2d(int dx_,int dy_)
{
	dx=dx_;
	dy=dy_;
	int size=dx*dy;
	data=new float[size];
	for(int i=0;i<size;i++)
		data[i]=xm_random_generator.frand();
}

Perlin2d::~Perlin2d()
{
	delete[] data;
}

void Perlin2d::setNumOctave(int num)
{
	int old_num=mul_octave.size();
	mul_octave.resize(num);

	float pow=1.0f/(1<<old_num);
	for(int i=old_num;i<num;i++)
	{
		mul_octave[i]=pow;
		pow*=0.5f;
	}
}

void Perlin2d::setOctave(int i,float mul)
{
	xassert(i>=0 && i<mul_octave.size());
	mul_octave[i]=mul;
}

float Perlin2d::get(float x,float y)
{
	float sum=0;
	for(int i=0;i<mul_octave.size();i++)
	{
		sum+=interpolate(x,y)*mul_octave[i];
		x*=2;y*=2;
	}
	return sum;
}


float Perlin2d::get_data(int x,int y)
{
	xassert(x>=0 && x<dx);
	xassert(y>=0 && y<dy);
	return data[x+y*dx];
}

float Perlin2d::interpolate(float x,float y)
{
	float cx,cy;
	int ix=remainder(x,dx,cx);
	int iy=remainder(y,dy,cy);
	int ix1=(ix+1)%dx;
	int iy1=(iy+1)%dy;

	float p00=get_data(ix,iy);
	float p01=get_data(ix1,iy);
	float p10=get_data(ix,iy1);
	float p11=get_data(ix1,iy1);

	return bilinear(p00,p01,p10,p11,cx,cy);
}
