#include "StdAfx.h"
#include "umath.h"
#include "Serialization\Serialization.h"
#include "XMath\SafeMath.h"

float HermitSpline(float t,float p0,float p1,float p2,float p3)
{
//	return (3.0f/2.0f*p1-3.0f/2.0f*p2-p0/2+p3/2)*t*t*t +
//			(2.0f*p2+p0-p3/2-5.0f/2.0f*p1)*t*t + (p2/2-p0/2)*t + p1;

	return (1.5f*(p1-p2)+(-p0+p3)*0.5f)*t*t*t +
		   (2.0f*p2+p0-p3*0.5f-2.5f*p1)*t*t + (p2-p0)*0.5f*t + p1;
}

float HermitSplineDerivation(float t,float p0,float p1,float p2,float p3)
{
	return (1.5f*(p1-p2)+(-p0+p3)*0.5f)*t*t*3 +
		   (2.0f*p2+p0-p3*0.5f-2.5f*p1)*t*2 + (p2-p0)*0.5f;
}

float HermitSplineDerivation2(float t,float p0,float p1,float p2,float p3)
{
	return (1.5f*(p1-p2)+(-p0+p3)*0.5f)*t*2*3 +
		   (2.0f*p2+p0-p3*0.5f-2.5f*p1)*2;
}

Vect3f HermitSpline(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3)
{
	Vect3f out;
	out.x=HermitSpline(t,p0.x,p1.x,p2.x,p3.x);
	out.y=HermitSpline(t,p0.y,p1.y,p2.y,p3.y);
	out.z=HermitSpline(t,p0.z,p1.z,p2.z,p3.z);
	return out;
}

Vect2f HermitSpline(float t,const Vect2f& p0,const Vect2f& p1,const Vect2f& p2,const Vect2f& p3)
{
	Vect2f out;
	out.x=HermitSpline(t,p0.x,p1.x,p2.x,p3.x);
	out.y=HermitSpline(t,p0.y,p1.y,p2.y,p3.y);
	return out;
}

Vect3f HermitSplineDerivation(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3)
{
	Vect3f out;
	out.x=HermitSplineDerivation(t,p0.x,p1.x,p2.x,p3.x);
	out.y=HermitSplineDerivation(t,p0.y,p1.y,p2.y,p3.y);
	out.z=HermitSplineDerivation(t,p0.z,p1.z,p2.z,p3.z);
	return out;
}

Vect3f HermitSplineDerivation2(float t,const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3)
{
	Vect3f out;
	out.x=HermitSplineDerivation2(t,p0.x,p1.x,p2.x,p3.x);
	out.y=HermitSplineDerivation2(t,p0.y,p1.y,p2.y,p3.y);
	out.z=HermitSplineDerivation2(t,p0.z,p1.z,p2.z,p3.z);
	return out;
}

void MatrixInterpolate(MatXf& out,const MatXf& a,const MatXf& b,float f)
{
	out.rot()[0][0]=LinearInterpolate(a.rot()[0][0],b.rot()[0][0],f);
	out.rot()[0][1]=LinearInterpolate(a.rot()[0][1],b.rot()[0][1],f);
	out.rot()[0][2]=LinearInterpolate(a.rot()[0][2],b.rot()[0][2],f);
	out.rot()[1][0]=LinearInterpolate(a.rot()[1][0],b.rot()[1][0],f);
	out.rot()[1][1]=LinearInterpolate(a.rot()[1][1],b.rot()[1][1],f);
	out.rot()[1][2]=LinearInterpolate(a.rot()[1][2],b.rot()[1][2],f);
	out.rot()[2][0]=LinearInterpolate(a.rot()[2][0],b.rot()[2][0],f);
	out.rot()[2][1]=LinearInterpolate(a.rot()[2][1],b.rot()[2][1],f);
	out.rot()[2][2]=LinearInterpolate(a.rot()[2][2],b.rot()[2][2],f);
	out.trans().x=LinearInterpolate(a.trans().x,b.trans().x,f);
	out.trans().y=LinearInterpolate(a.trans().y,b.trans().y,f);
	out.trans().z=LinearInterpolate(a.trans().z,b.trans().z,f);
}

void MatrixGrandSmittNormalizationYZ(Mat3f& out,Vect3f direction_front,Vect3f direction_up)
{
	Vect3f y = direction_front.normalize();
	Vect3f x = direction_up%y;
	x=(x-y*y.dot(x));
	x.normalize();
	Vect3f z = x%y;

	out=Mat3f(
	x.x, y.x, z.x,
	x.y, y.y, z.y,
	x.z, y.z, z.z
	);

	if(x.norm2()<=sqr(FLT_EPS))
		out=Mat3f::ID;

	xassert(isEq(out.det(), 1.f, 0.01f));
}

void sPolygon::serialize(Archive& ar)
{
	MergeBlocksAuto mergeBlock(ar);
	ar.serialize(p1, "p1", "p1");
	ar.serialize(p2, "p2", "p2");
	ar.serialize(p3, "p3", "p3");
}

