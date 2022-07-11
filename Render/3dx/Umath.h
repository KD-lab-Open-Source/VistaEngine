#ifndef __UMATH_H__
#define __UMATH_H__

#include <windows.h>
#include <math.h>
#include "XMath\xmath.h"

class Archive;

struct sPolygon
{
	WORD p1,p2,p3;
	WORD& operator[](int i) {return *(i+&p1);}
	void set(WORD i1,WORD i2,WORD i3) { p1=i1; p2=i2; p3=i3; }
	void serialize(Archive& ar);
};

//Если число - степень двойки, возвращает эту степень.
inline int ReturnBit(int a)
{
	int count=0;
	while((a>>=1)>=1) count++;
	return count;
}

//Проверяет - является ли число степенью двойки и больше нуля.
inline bool IsPositivePower2(int a)
{
	return !((a-1)&a) && a>0;
}

//Возвращает минимальное число,являющееся степенью двойки и не меньше, чем n
inline int Power2up(int n)
{
	int i=1;
	while(i<n)
		i=i<<1;
	return i;
}


inline float LinearInterpolate(float a,float b,float x)
{
	return a+(b-a)*x;
}

inline float CosInterpolate(float a,float b,float x)
{
	float f=(1-cosf(x*3.14159f))*0.5f;
	return a*(1-f)+b*f;
}

inline float ArcLengthInterpolate(float a,float b,float x)
{
	return a+((1-x)*x+x)*(b-a);
}

inline float QuadratInterpolate(float v0,float v1,float v2,float x) // v0 <= x <= v1 <= v2
{
	float b=(4*v1-3*v0-v2)*0.5f;
	float a=v1-v0-b;
	float c=v0;
	return (a*x+b)*x+c;
}

inline float CubicInterpolate(float v0,float v1,float v2,float v3,float x) // v0 <= v1 <= x <= v2 <= v3
{
	float p=(v3-v2)-(v0-v1);
	float q=(v0-v1)-p;
	float r=v2-v0;
	float s=v1;
	return ((p*x+q)*x+r)*x+s;
}

void MatrixInterpolate(MatXf& out,const MatXf& a,const MatXf& b,float f);

float HermitSpline(float t,float p0,float p1,float p2,float p3);
Vect3f HermitSpline(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3);
Vect2f HermitSpline(float t,const Vect2f& p0,const Vect2f& p1,const Vect2f& p2,const Vect2f& p3);
float HermitSplineDerivation(float t,float p0,float p1,float p2,float p3);
Vect3f HermitSplineDerivation(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3);
float HermitSplineDerivation2(float t,float p0,float p1,float p2,float p3);
Vect3f HermitSplineDerivation2(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3);

inline float bilinear(const float p00, const float p01, const float p10, const float p11, float cx,float cy)
{
	float p0=p00*(1-cx)+p01*cx;
	float p1=p10*(1-cx)+p11*cx;
	float p=p0*(1-cy)+p1*cy;
	return p;
}

//Разница в том, что считать единицей = 255 или 256
inline BYTE ByteInterpolate256(BYTE a,BYTE b,BYTE factor)
{
	return a+((int(b-a))*int(factor)>>8);
}
inline BYTE ByteInterpolate(BYTE a,BYTE b,BYTE factor)
{
	return a+(int(b-a))*int(factor)/255;
}

inline BYTE bilinear(BYTE p00,BYTE p01,BYTE p10,BYTE p11,BYTE cx,BYTE cy)
{
	BYTE p0=ByteInterpolate256(p00,p01,cx);
	BYTE p1=ByteInterpolate256(p10,p11,cx);
	BYTE p=ByteInterpolate256(p0,p1,cy);
	return p;
}

//Строит ортогональную матрицу, ось y которой совпадает с direction_front
//ось z практически перпендикулярна direction_up
void MatrixGrandSmittNormalizationYZ(Mat3f& out,Vect3f direction_front,Vect3f direction_up);

#endif // __UMATH_H__
