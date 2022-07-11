#ifndef __PERLIN_H__
#define __PERLIN_H__

class Perlin3d
{
public:
	Perlin3d(int dx,int dy,int dz);
	~Perlin3d();

	void setNumOctave(int num);
	void setOctave(int i,float mul);

	float get(float x,float y,float z);
protected:
	int dx,dy,dz;
	float* data;
	vector<float> mul_octave;

	float interpolate(float x,float y,float z);

	float get_data(int x,int y,int z);
};

class Perlin2d
{
public:
	Perlin2d(int dx,int dy);
	~Perlin2d();

	void setNumOctave(int num);
	void setOctave(int i,float mul);

	float get(float x,float y);
protected:
	int dx,dy;
	float* data;
	vector<float> mul_octave;

	float interpolate(float x,float y);

	float get_data(int x,int y);
};

#endif  __PERLIN_H__
