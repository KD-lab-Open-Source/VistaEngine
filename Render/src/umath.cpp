#include "StdAfxRD.h"
#include "umath.h"
#include "Serialization.h"

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

inline float TriangleArea(float x1,float y1,float x2,float y2,float x3,float y3)
{ // формула Герона
	float a=(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
	if(a<0) return 0;
	a=sqrtf(a);
	float b=(x3-x1)*(x3-x1)+(y3-y1)*(y3-y1);
	if(b<0) return 0;
	b=sqrtf(b);
	float c=(x2-x3)*(x2-x3)+(y2-y3)*(y2-y3);
	if(c<0) return 0;
	c=sqrtf(c);
	float p=(a+b+c)*0.5f;
	p=p*(p-a)*(p-b)*(p-c);
	if(p<0) return 0;
	return sqrtf(p);
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

void CheckMatrix(const MatXf& Matrix)
{
	const float good_trans=1e6f;
	const float good_rot=1e5f;
	VISASSERT(Matrix.trans().x>-good_trans && Matrix.trans().x<good_trans);
	VISASSERT(Matrix.trans().y>-good_trans && Matrix.trans().y<good_trans);
	VISASSERT(Matrix.trans().z>-good_trans && Matrix.trans().z<good_trans);

	VISASSERT(Matrix.rot().xrow().x>-good_rot && Matrix.rot().xrow().x<good_rot);
	VISASSERT(Matrix.rot().xrow().y>-good_rot && Matrix.rot().xrow().y<good_rot);
	VISASSERT(Matrix.rot().xrow().z>-good_rot && Matrix.rot().xrow().z<good_rot);

	VISASSERT(Matrix.rot().yrow().x>-good_rot && Matrix.rot().yrow().x<good_rot);
	VISASSERT(Matrix.rot().yrow().y>-good_rot && Matrix.rot().yrow().y<good_rot);
	VISASSERT(Matrix.rot().yrow().z>-good_rot && Matrix.rot().yrow().z<good_rot);

	VISASSERT(Matrix.rot().zrow().x>-good_rot && Matrix.rot().zrow().x<good_rot);
	VISASSERT(Matrix.rot().zrow().y>-good_rot && Matrix.rot().zrow().y<good_rot);
	VISASSERT(Matrix.rot().zrow().z>-good_rot && Matrix.rot().zrow().z<good_rot);
}

void sColor4c::serialize(Archive& ar) 
{
	ar.serialize(r, "r", "&r");
	ar.serialize(g, "g", "&g");
	ar.serialize(b, "b", "&b");
	ar.serialize(a, "a", "&a");
}

void sColor4f::serialize(Archive& ar) 
{
	ar.serialize(r, "r", "&r");
	ar.serialize(g, "g", "&g");
	ar.serialize(b, "b", "&b");
	ar.serialize(a, "a", "&a");
}

void sColor3c::serialize(Archive& ar) 
{
	ar.serialize(r, "r", "&r");
	ar.serialize(g, "g", "&g");
	ar.serialize(b, "b", "&b");
}

void sBox6f::SetBox(Vect3f* array,int size)
{
	SetInvalidBox();
	for(int ibox=0;ibox<size;ibox++)
	{
		AddBound(array[ibox]);
	}
}

void MatrixGrandSmittNormalizationXZ(Mat3f& out,Vect3f direction_front,Vect3f direction_up)
{
	Vect3f x = direction_front.Normalize();
	Vect3f y = direction_up%x;
	y=(y-x*x.dot(y));
	y.Normalize();
	Vect3f z = x%y;

	out=Mat3f(
	x.x, y.x, z.x,
	x.y, y.y, z.y,
	x.z, y.z, z.z
	);

#ifndef _FINAL_VERSION_
	float det=out.det();
	xassert(det>=0.99 && det<=1.01);
#endif
}


void MatrixGrandSmittNormalizationYZ(Mat3f& out,Vect3f direction_front,Vect3f direction_up)
{
	Vect3f y = direction_front.Normalize();
	Vect3f x = direction_up%y;
	x=(x-y*y.dot(x));
	x.Normalize();
	Vect3f z = x%y;

	out=Mat3f(
	x.x, y.x, z.x,
	x.y, y.y, z.y,
	x.z, y.z, z.z
	);

	if(x.norm2()<=sqr(FLT_EPS))
		out=Mat3f::ID;
#ifndef _FINAL_VERSION_
	float det=out.det();
	xassert(det>=0.99 && det<=1.01);
#endif
}

const sRectangle4f sRectangle4f::ID(0.0f, 0.0f, 1.0f, 1.0f);

sBox6f sBox6f::BAD(1e30f,1e30f,1e30f,-1e30f,-1e30f,-1e30f);


bool sBox6f::isCrossOrInside(const Se3f& pose, const Vect3f& p0, const Vect3f& p1) const
{
	Vect3f pointBegin;
	Vect3f pointEnd(center());
	pose.xformPoint(pointEnd);
	pointBegin.sub(p0, pointEnd);
	pose.invXformVect(pointBegin);
	pointEnd.sub(p1, pointEnd);
	pose.invXformVect(pointEnd);

	float t_begin, t_end;
	bool inited = false;

	for(int i = 0; i < 3; ++i)
	{
		float begin = pointBegin[i], end = pointEnd[i];
		float halfSize = (max[i] - min[i]) / 2.f;
		
		if(begin < end) {
			if(begin > halfSize || end < -halfSize)
				return false;
			float shift = end - begin;
			if(shift > FLT_EPS) {
				float beginTemp = (-halfSize - begin) / shift;
				if(!inited || beginTemp > t_begin)
					t_begin = beginTemp;
				float endTemp = (halfSize - begin) / shift;
				if(!inited || endTemp < t_end)
					t_end = endTemp;
				inited = true;
			}

		}
		else {
			if(end > halfSize || begin < -halfSize)
				return false;
			float shift = end - begin;
			if(shift < -FLT_EPS) {
				float beginTemp = (halfSize - begin) / shift;
				if(!inited || beginTemp > t_begin)
					t_begin = beginTemp;
				float endTemp = (-halfSize - begin) / shift;
				if(!inited || endTemp < t_end)
					t_end = endTemp;
				inited = true;
			}
		}
	}

	return !inited || t_end >= t_begin;
}
