#include "StdAfx.h"
#include "InvertMatrix.h"
#include "XMath\Mat4f.h"

void calcLinear(vector<float>& data,float& a0,float& a1)
{
	//x=-0.5..+0.5
	//S(xi)=0
	xassert(data.size() >= 2);
	int N = data.size();
	double Sy=0,Sx2=0,Sxy=0;
	for(int i = 0; i < N; i++){
		double x = i/double(N-1) - 0.5;
		double y = data[i];
		Sy+=y;
		Sx2+=x*x;
		Sxy+=x*y;
	}
	a0=Sy/N;
	a1=Sxy/Sx2;
}

void calcQuadrix(vector<float>& data,float& a0,float& a1,float& a2)
{
	//x=-0.5..+0.5
	//S(xi)=0
	//S(xi^3)=0
	xassert(data.size() >= 3);
	int N = data.size();
	double Sx2 = 0;
	double Sx4 = 0; 
	double Sxy = 0;
	double Sx2y = 0;
	double Sy = 0;
	for(int i=0;i < N;i++){
		double x = i/double(N - 1) - 0.5;
		double x2 = sqr(x);
		double y = data[i];
		Sx2 += x2;
		Sx4 += sqr(x2);
		Sy += y;
		Sxy += x*y;
		Sx2y += x2*y;
	}

	double t1 = Sx2*Sx2;
	double t10 = t1*Sx2-Sx4*N*Sx2;
	xassert(fabsf(t10) > FLT_EPS);
	t10 = 1.f/t10;
	a0 = -(-t1*Sx2y+Sy*Sx4*Sx2)*t10;
	a1 = (t1*Sxy-Sxy*N*Sx4)*t10;
	a2 = (t1*Sy-Sx2y*N*Sx2)*t10;
}

void calcCubic(vector<float>& data,float& a0,float& a1,float& a2,float& a3)
{
	//x=-0.5..+0.5
	//S(xi)=0
	//S(xi^3)=0
	//S(xi^5)=0
	xassert(data.size() >= 4);
	int N = data.size();
	double Sx2 = 0;
	double Sx4 = 0; 
	double Sx6 = 0;
	double Sxy = 0;
	double Sx2y = 0;
	double Sx3y = 0;
	double Sy = 0;
	for(int i=0;i < N;i++){
		double x = i/double(N - 1) - 0.5;
		double x2 = sqr(x);
		double y = data[i];
		Sx2 += x2;
		Sx4 += sqr(x2);
		Sx6 += x2*sqr(x2);
		Sy += y;
		Sxy += x*y;
		Sx2y += x2*y;
		Sx3y += x*x2*y;
	}

	double t2 = Sx6*Sx4;
	double t4 = Sx4*Sx4;
	double t5 = Sx2*t4;
	double t7 = Sx2*Sx2;
	double t10 = t4*Sx4;
	double t13 = N*Sx2;
	double t16 = t7*Sx2;
	double t20 = -t13*t2+N*t10+Sx6*t16-t7*t4;
	xassert(fabsf(t20) > FLT_EPS);
	t20 = 1.f/t20;
	double t24 = N*t4;
	double t26 = t7*Sx4;
	a0 = -(Sy*Sx2*t2+t5*Sx2y-Sx2y*t7*Sx6-Sy*t10)*t20;
	a1 = -(N*Sxy*t2-t24*Sx3y+t26*Sx3y-t7*Sxy*Sx6)*t20;
	a2 = -(-t24*Sx2y+N*Sx2y*Sx2*Sx6+t5*Sy-t7*Sy*Sx6)*t20;
	a3 = (-t13*Sx4*Sx3y+t24*Sxy+Sx3y*t16-t26*Sxy)*t20;

/*	if(N == 4){
		for(int i=0;i < N;i++){
			double x = i/float(N-1)-0.5f;
			double x2 = sqr(x);
			double y = data[i];
			double f = a0 + a1*x + a2*x2 + a3*x*x2; 
			double d = fabsf(y - f); 
			xassert(d <	1e-6);
		}
	}*/
}

void ScaleSpline(float& a0,float& a1,float& a2,float& a3,float tscale,float toffset)
{
	float ts=1/tscale,ts2=ts*ts,ts3=ts2*ts;
	float to=-toffset*ts,to2=to*to,to3=to2*to;
	a0=a0+a1*to+a2*to2+a3*to3;
	a1=ts*(a1+a2*2*to+a3*3*to2);
	a2=ts2*(a2+a3*3*to);
	a3=ts3*a3;
}
