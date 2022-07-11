#include "StdAfxRD.h"
#include "mats.h"

void xform(const QuatF& q,const Vect3f& v, Vect3f& out)
{
//  Vect3f *u, uv, uuv;
	Vect3f uv, uuv;
//  u = (Vect3f *) &x_;
//  uv.cross(*u, v);uv.scale(2.f);
  uv.x = 2.f*(q.y_ * v.z - q.z_ * v.y);
  uv.y = 2.f*(q.z_ * v.x - q.x_ * v.z);
  uv.z = 2.f*(q.x_ * v.y - q.y_ * v.x);

//  uuv.cross(*u, uv);
  uuv.x = q.y_ * uv.z - q.z_ * uv.y;
  uuv.y = q.z_ * uv.x - q.x_ * uv.z;
  uuv.z = q.x_ * uv.y - q.y_ * uv.x;
//uv.scale(s_);
  uv.x*=q.s_;
  uv.y*=q.s_;
  uv.z*=q.s_;
//
  out.add(v, uv);
  out.add(uuv);
}

void Mats::mult(const Mats& t,const Mats& u)
{
	s=t.s*u.s;
	q.mult(t.q,u.q);
	xform(t.q,u.d,d);
	d*=t.s;
	d+=t.d;
}

void Mats::copy_right(MatXf& mat) const
{
	float s2=s*2;
	mat.R.xx = s2 * (q.s_ * q.s_ + q.x_ * q.x_ - 0.5f);
	mat.R.yy = s2 * (q.s_ * q.s_ + q.y_ * q.y_ - 0.5f);
	mat.R.zz = s2 * (q.s_ * q.s_ + q.z_ * q.z_ - 0.5f);

	mat.R.xy = s2 * (q.y_ * q.x_ - q.z_ * q.s_);
	mat.R.yx = s2 * (q.x_ * q.y_ + q.z_ * q.s_);


	mat.R.yz = s2 * (q.z_ * q.y_ - q.x_ * q.s_);
	mat.R.zy = s2 * (q.y_ * q.z_ + q.x_ * q.s_);

	mat.R.zx = s2 * (q.x_ * q.z_ - q.y_ * q.s_);
	mat.R.xz = s2 * (q.z_ * q.x_ + q.y_ * q.s_);

	mat.d=d;
}


void Mats::Identify()
{
	se()=Se3f::ID;
	s=1;
}

static float c00=0.698627f,c01=1.104161f,c02=-0.795403f,c03=-0.413569f;
static float c10=-0.259991f,c11=-0.119657f,c12=1.012162f,c13=0.458489f;
static float c20=0.038053f,c21=0.010083f,c22=-0.088419f,c23=-0.005936f;
static float c30=0.023756f,c31=0.005421f,c32=-0.130263f,c33=-0.039266f;

void slerp_fast(QuatF& out,const QuatF& a,const QuatF& b,float t)
{ 
	float scale0, scale1;

	// calc cosine
	float cosom = a.dot(b);

	// adjust signs (if necessary)
	if(cosom < 0.0){ 
		cosom = -cosom; 
		out.negate(b);
	}else
		out = b;

	t-=0.5f;
	float a0=c00+c02*t*t;
	float a1=c10+c12*t*t;
	float a2=c20+c22*t*t;
	float a3=c30+c32*t*t;
	float b0=c01*t+c03*t*t*t;
	float b1=c11*t+c13*t*t*t;
	float b2=c21*t+c23*t*t*t;
	float b3=c31*t+c33*t*t*t;

	float asum=a0+a1*cosom+a2*cosom*cosom+a3*cosom*cosom*cosom;
	float bsum=b0+b1*cosom+b2*cosom*cosom+b3*cosom*cosom*cosom;

	scale0=asum-bsum;
	scale1=asum+bsum;

	out *= scale1;
	//out += a*scale0;
	out.s() += a.s()*scale0;
	out.x() += a.x()*scale0;
	out.y() += a.y()*scale0;
	out.z() += a.z()*scale0;
}


void lerp(QuatF& out,const QuatF& a,const QuatF& b,float t)
{ 
	// Slerp(q1,q2,t) = (sin((1-t)*A)/sin(A))*q1+(sin(t*A)/sin(A))*q2 
	float scale0, scale1;

	// calc cosine
	float cosom = a.dot(b);

	// adjust signs (if necessary)
	if(cosom < 0.0){ 
		cosom = -cosom; 
		out.negate(b);
	}
	else
		out = b;

	// calculate coefficients
	scale0 = 1.0f - t;
	scale1 = t;

        // calculate final values
	out *= scale1;
	out.s() += a.s()*scale0;
	out.x() += a.x()*scale0;
	out.y() += a.y()*scale0;
	out.z() += a.z()*scale0;
	out.normalize();
	//fast_normalize(out);
}


void Mats::set(MatXf mat)
{
	s=mat.rot().xrow().norm();
	float invs=1/s;
	mat.rot()*=invs;
	se().set(mat);
	scale()=s;
}
