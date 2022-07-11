#include "StdAfx.h"
#include "InvertMatrix.h"

class Vect4d
{
public:
	double x,y,z,w;

	xm_inline double& operator[](int i){return *(((double*) &x) + i);}
};

class Mat4d
{

  // (stored in row-major order)
  double xx, xy, xz, xw,
       yx, yy, yz, yw,
       zx, zy, zz, zw,
       wx, wy, wz, ww;

public:
	// index-based access:  0=xrow, 1=yrow, 2=zrow, 3=wrow.
	xm_inline const Vect4d& operator[](int i) const {return *(((Vect4d *) &xx) + i);}
	xm_inline Vect4d& operator[](int i)       {return *(((Vect4d *) &xx) + i);}

	Vect4d& xform(const Vect4d& v, Vect4d& xv) const
	{
		xv.x = xx * v.x + xy * v.y + xz * v.z + xw*v.w;
		xv.y = yx * v.x + yy * v.y + yz * v.z + yw*v.w;
		xv.z = zx * v.x + zy * v.y + zz * v.z + zw*v.w;
		xv.w = wx * v.x + wy * v.y + wz * v.z + ww*v.w;
		return xv;
	}

	Vect4d& xform(Vect4d& v) const
	{
		Vect4d v0 = v;
		return xform(v0, v);
	}

	xm_inline friend Vect4d operator* (const Mat4d& M, const Vect4d& v) { Vect4d xv; return M.xform(v, xv); }
};

/*
bool invertmatrix(Mat4f& b)
{
	real epsilon=1e-5f;
	int n=4;
    int j;
    Mat4f a=b;
	Mat4f out;
	for(int i=1;i<=n;i++)
	{
		for(int j=1;j<=n;j++)
			out(i,j)=0;
		out(i,i)=1;
	}

	//a(i,j) i-столец, j- строка

    for(i=1;i<=n;i++)
    {
        int k = i;
		real fkmax=a(k,i);
		int kmax=k;

        for(;k<=n;k++)
        {
            if( fabs(a(k,i))>fkmax )
            {
				fkmax=fabs(a(k,i));
				kmax=k;
            }
        }
		k=kmax;
		if(!(fabs(fkmax)>epsilon))
			return false;

        if( k!=i )
        {
            for(j=1;j<=n;j++)
            {
                swap(a(k,j),a(i,j));
				swap(out(k,j),out(i,j));
            }
        }

		for(j = 1;j<=n;j++)
		{
			out(i,j)/=a(i,i);
		}

		for(j = n;j>=i;j--)
        {
            a(i,j) = a(i,j)/a(i,i);
        }

        for(k=1;k<=n;k++)
        {
            if( k!=i )
            {
                for(j=1;j<=n;j++)
                {
                    out(k,j) = out(k,j)-out(i,j)*a(k,i);
                }

                for(j=n;j>=i;j--)
                {
                    a(k,j) = a(k,j)-a(i,j)*a(k,i);
                }
            }
        }
    }

	b=out;
    return true;
}
*/

template<class Matrix>
bool invertmatrix_zz(Matrix& b,int n)
{
	real epsilon=1e-5f;
    int j;
    Matrix a=b;
	Matrix out;
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
			out[i][j]=0;
		out[i][i]=1;
	}

	//a(i,j) i-столец, j- строка

    for(i=0;i<n;i++)
    {
        int k = i;
		real fkmax=a[k][i];
		int kmax=k;

        for(;k<n;k++)
        {
            if( fabs(a[k][i])>fkmax )
            {
				fkmax=fabs(a[k][i]);
				kmax=k;
            }
        }
		k=kmax;
		if(!(fabs(fkmax)>epsilon))
			return false;

        if( k!=i )
        {
            for(j=0;j<n;j++)
            {
                swap(a[k][j],a[i][j]);
				swap(out[k][j],out[i][j]);
            }
        }

		real inv_aii=1/a[i][i];
		for(j = 0;j<n;j++)
		{
			out[i][j]*=inv_aii;
		}

		for(j = n-1;j>=i;j--)
        {
            a[i][j] = a[i][j]*inv_aii;
        }

        for(k=0;k<n;k++)
        {
            if( k!=i )
            {
                for(j=0;j<n;j++)
                {
                    out[k][j] = out[k][j]-out[i][j]*a[k][i];
                }

                for(j=n-1;j>=i;j--)
                {
                    a[k][j] = a[k][j]-a[i][j]*a[k][i];
                }
            }
        }
    }

	b=out;
    return true;
}

bool invertmatrix(Mat4f& b)
{
	return invertmatrix_zz(b,4);
}

bool invertmatrix(Mat3f& b)
{
	return invertmatrix_zz(b,3);
}

bool invertmatrix(Mat4d& b)
{
	return invertmatrix_zz(b,4);
}

//Матрица для среднеквадратического приближения для функции
//y=a0+a1*x+a2*x*x+a3*x*x*x на интервале -0.5..+0.5
void MinQuadMatrix(Mat4f& out,int num)
{
	int n=4;
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
		{
			int kmax=i+j;

			real sum=0;
			for(int r=0;r<num;r++)
			{
				real f=r/real(num-1)-0.5f;
				real power=1;
				for(int k=0;k<kmax;k++)
					power*=f;
				sum+=power;
			}

			real acur=sum/num;
			out[i][j]=acur;
		}
	}

	invertmatrix(out);
}

void MinQuadMatrix(Mat3f& out,int num)
{
	int n=3;
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
		{
			int kmax=i+j;

			real sum=0;
			for(int r=0;r<num;r++)
			{
				real f=r/real(num-1)-0.5f;
				real power=1;
				for(int k=0;k<kmax;k++)
					power*=f;
				sum+=power;
			}

			real acur=sum/num;
			out[i][j]=acur;
		}
	}

	out.invert();
}

void MinQuadMatrixSimply(Mat4f& out,int num)
{
//Используется тот факт, что у нечётных степеней сумма нулевая
//разбивается на 2 матрицы
	xassert(num>3);
	Mat2f m[2];
//m[0]
//00,02
//20,22
//m[1]
//11,13
//31,33
	for(int imat=0;imat<2;imat++)
	{
		for(int i=0;i<2;i++)
		{
			for(int j=0;j<2;j++)
			{
				int kmax=(i+j+imat)*2;

				real sum=0;
				for(int r=0;r<num;r++)
				{
					real f=r/real(num-1)-0.5f;
					real power=1;
					for(int k=0;k<kmax;k++)
						power*=f;
					sum+=power;
				}

				real acur=sum/num;
				m[imat][i][j]=acur;
			}
		}

		m[imat].Invert();
	}
	out[0][0]=m[0][0][0]; out[0][1]=0;
	out[0][2]=m[0][0][1]; out[0][3]=0;
	out[1][0]=0; out[1][1]=m[1][0][0];
	out[1][2]=0; out[1][3]=m[1][0][1];
	out[2][0]=m[0][1][0]; out[2][1]=0;
	out[2][2]=m[0][1][1]; out[2][3]=0;
	out[3][0]=0; out[3][1]=m[1][1][0];
	out[3][2]=0; out[3][3]=m[1][1][1];
}

void CalcLagrange(vector<real>& data,real& a0,real& a1,real& a2,real& a3)
{
	int size=data.size();

	Vect4f c;
	Vect4f func(0,0,0,0);
	Mat4f mat;
	if(size==3)
	{
		Mat3f m3;
		MinQuadMatrix(m3,size);
		int i,j;
		for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			mat[i][j]=0;

		for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			mat[i][j]=m3[i][j];
	}else
	{
		MinQuadMatrixSimply(mat,size);
	}

	real len=(real)(size-1);
	for(int i=0;i<size;i++)
	{
		real t=i/real(size-1)-0.5f;
		real f=data[i];
		func[0]+=f;
		func[1]+=f*t;
		func[2]+=f*t*t;
		func[3]+=f*t*t*t;
	}

	func[0]/=size;
	func[1]/=size;
	func[2]/=size;
	func[3]/=size;
	c=mat*func;

	a0=c[0];
	a1=c[1];
	a2=c[2];
	a3=c[3];
}

void CalcLinear(vector<real>& data,real& a0,real& a1)
{
/*
	производная(sum(a0+a1*xi-yi)^2)
a0: P(a0^2+2*a0*(a1*xi-yi)-yi^2)=2*a0+2*(a1*xi-yi)=0                sum(a0+a1*xi-yi)=0;
a1: P(a1^2*xi^2+2*a1*xi*(a0-yi)+(a0-yi)^2)=2*a1*xi^2+2*xi*(a0-yi)=0 sum(a1*xi^2+xi*(a0-yi))=0
		a0*N+a1*S(xi)-S(yi)=0;
	    a0*S(xi)+a1*S(xi^2)-S(xi*yi)=0;

	a0*p00+a1*p01=c0
	a0*p10+a1*p11=c1
	a0=(c0-a1*p01)/p00
	(c0-a1*p01)/p00*p10+a1*p11=c1
	(c0-a1*p01)*p10+a1*p11*p00=c1*p00
	a1*(p11*p00-p01*p10)=c1*p00-c0*p10
	a1=(c1*p00-c0*p10)/(p11*p00-p01*p10)
	a0=(c0-(c1*p00-c0*p10)/(p11*p00-p01*p10)*p01)/p00=
	   (c0*(p11*p00-p01*p10)-(c1*p00-c0*p10)*p01)/p00/(p11*p00-p01*p10)=
	   (c0*(p11*p00-p01*p10+p10*p01)-c1*p00*p01)/p00/(p11*p00-p01*p10)=
	   (c0*p11-c1*p01)/(p11*p00-p01*p10)

	a0=( c0*p11-c1*p01)/(p11*p00-p01*p10)
	a1=(-c0*p10+c1*p00)/(p11*p00-p01*p10)
*/
	//x=-0.5..+0.5
	//S(xi)=0
	int N=data.size();
	real Sy=0,Sx2=0,Sxy=0,Sx=0;
	for(int i=0;i<N;i++)
	{
		real x=i/real(N-1)-0.5f;
		real y=data[i];
		Sx+=x;
		Sy+=y;
		Sx2+=x*x;
		Sxy+=x*y;
	}
	xassert(fabsf(Sx)<1e-4f);
	a0=Sy/N;
	a1=Sxy/Sx2;
}

/////////////////lagrange с весами///////////
void MinQuadMatrixSimplyWeight(Mat4d& out,vector<real>& weight)
{
//Используется тот факт, что у нечётных степеней сумма нулевая
//разбивается на 2 матрицы
	int num=weight.size();
	xassert(num>3);
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			int kmax=(i+j);

			double sum=0;
			double div=0;
			for(int r=0;r<num;r++)
			{
				double f=r/double(num-1)-0.5f;
				double power=1;
				for(int k=0;k<kmax;k++)
					power*=f;
				power*=weight[r];
				sum+=power;
				div+=weight[r];
			}

			double acur=sum/div;
			out[i][j]=acur;
		}
	}

	invertmatrix(out);
}

void CalcLagrangeWeight(vector<real>& data,vector<real>& weight,real& a0,real& a1,real& a2,real& a3)
{
	int size=data.size();
	xassert(size==weight.size());

	Vect4d c;
	Vect4d func;
	func.x=func.y=func.z=func.w=0;
	Mat4d mat;
	if(size<=3)
	{
		xassert(0);
	}else
	{
		MinQuadMatrixSimplyWeight(mat,weight);
		//MinQuadMatrixSimply(mat,size);
	}

	real len=(real)(size-1);
	real div=0;
	for(int i=0;i<size;i++)
	{
		real t=i/real(size-1)-0.5f;
		real f=data[i];
		f*=weight[i];
		div+=weight[i];
		func[0]+=f;
		func[1]+=f*t;
		func[2]+=f*t*t;
		func[3]+=f*t*t*t;
	}

	func[0]/=div;
	func[1]/=div;
	func[2]/=div;
	func[3]/=div;
	c=mat*func;

	a0=c[0];
	a1=c[1];
	a2=c[2];
	a3=c[3];
}
