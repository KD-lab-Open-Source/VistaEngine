//	2000 Balmer (Poskryakov Dmitry) 
#include "StdAfx.h"
#include "Bound.h"
//#include <string.h>
#ifndef _FINAL_VERSION_
#define VERIFY(exp) xxassert(exp, #exp)
#else
#define VERIFY(exp) exp
#endif

///////////////////////////////////////////////////////////////////////////////////////
void Bound::New()
{
	yrange=NULL;
	xassert(dy>=0);
	if(dy==0)return;
	yrange=new vBoundRange[dy];
}
void Bound::Delete()
{
	delete[] yrange;
}

Bound::Bound()
{
	basey=dy=0;
	New();
}
Bound::Bound(int _basey,int _dy)
{
	basey=_basey;
	dy=_dy;
	New();
}
Bound::~Bound()
{
	Delete();
}

void Bound::NewSize(int _basey,int _dy)
{
	Delete();
	basey=_basey;
	dy=_dy;
	New();
}

void Bound::InitEmpty(int _basey,int _dy)
{
	NewSize(_basey,_dy);
}

void Bound::Compress(int flag)
{
	if(flag&COMP_SUM)
	{
		for(int i=0;i<dy;i++)
		{
			vBoundRange& a=yrange[i];
			if(a.empty())
				continue;
			int a_n=a.size();
			int a_nn=a_n;
			int old_max=a[0].xmax;

			int k=1;
			for(int j=1;j<a_n;j++)
			{
				int t_xmin=a[j].xmin;
				int t_xmax=a[j].xmax;

				if(t_xmin==t_xmax &&(j<a_n))
				{
					a_nn--;
					continue;
				}

				if(old_max==t_xmin)						
				{
					a[k-1].xmax=t_xmax;
					a_nn--;
				}else
				{
					a[k].xmin=t_xmin;
					a[k++].xmax=t_xmax;
				}

				old_max=t_xmax;
			}
			xassert(a_nn>=0);
			a.resize(a_nn);
			//тут возможно swap сделать для реальной очистки памяти.
		}
	}

	if(flag&COMP_REALLOC)
	{
		int ymin,ymax;
		for(ymin=0;ymin<dy;ymin++)
		{
			if(yrange[ymin].size())break;
		}
		for(ymax=dy-1;ymax>=0;ymax--)
		{
			if(yrange[ymax].size())break;
		}

		int _dy=ymax-ymin+1;

		//if(dy!=_dy)xassert(0);

		if(dy!=_dy)
		{
			VERIFY(Realloc(basey+ymin,_dy));
		}
	}
}

bool Bound::Realloc(int _basey,int _dy,bool proverka)
{
	int i;
#ifdef _DEBUG
	if(!proverka)goto Ok;
	xassert(dy>0);
//	xassert(_basey+_dy<=480);
//	xassert(heavy<20);
	//Можно ли корректно переаллокировать
	if(IsEmpty()) goto Ok;
	//Не теряется ли что по нижней границе ymin
	if(_basey>basey)
	{
		if(_basey>=basey+dy)
			return false;

		int maxy=_basey-basey;
		for(int i=0;i<maxy;i++)
		{
			if(yrange[i].size()>0)return false;
		}
	}
		
	//Не теряется ли что по верхней границе ymax	
	if(_basey+_dy<basey+dy)
	{
		if(_basey+_dy<=basey)
			return false;
		int miny=_basey+_dy-basey;
		for(int i=miny;i<dy;i++)
		{
			if(yrange[i].size()>0)
				return false;
		}
	}
Ok:
#endif _DEBUG

	vBoundRange *ynew=new vBoundRange[_dy];
	
	int miny=max(_basey,basey);
	int maxy=min(_basey+_dy,basey+dy);
	int ddy=maxy-miny;
	int b1=miny-basey;
	int b2=miny-_basey;

	xassert(ddy<=dy);
	xassert(b1>=0 && b1+ddy<=dy);
	xassert(b2>=0 && b2+ddy<=_dy);
	for(i=0;i<ddy;i++)
	{
		int bb1=(b1+i);
		int bb2=(b2+i);
		ynew[bb2]=yrange[bb1];
	}

	Delete();
	yrange=ynew;

	basey=_basey;
	dy=_dy;

	return true;
}

void Bound::CopyLine(int i,//Куда копировать
		vBoundRange& from//Откуда копировать
		)
{
	xassert(i>=0 && i<dy);
	yrange[i]=from;
}
void Bound::CopyLine(int i,//Куда копировать
		Bound& b,int j//Откуда копировать
		)
{
	xassert(j>=0 && j<b.dy);
	CopyLine(i,b.yrange[j]);
}

void Bound::OnLineOr(int ii,vBoundRange& b)
{
	int j,k;

	vBoundRange& a=yrange[ii];
	int a_n=a.size();
	int b_n=b.size();

	temp.clear();
	j=k=0;

	int flag=0;
	int xa;
	while( j<a_n && k<b_n)
	{
		if(flag)
		{
			if(a[j].xmax>b[k].xmax)
			{
				xa=a[j].xmax;
				while(k<b_n && b[k].xmax<=xa)k++;
				if(k<b_n)
				  if(b[k].xmin<=xa)
				    continue;
				j++;
			} else
			{
				xa=b[k].xmax;
				while(j<a_n && a[j].xmax<=xa)j++;
				if(j<a_n)
				  if(a[j].xmin<=xa)
				    continue;
				k++;
			}

			temp.back().xmax=xa;
			flag=0;
		} else
		{
			if(a[j].xmin<b[k].xmin)
			{
				temp.push_back(BoundRange());
				temp.back().xmin=a[j].xmin;
				if(a[j].xmax<b[k].xmin)
				{
					temp.back().xmax=a[j].xmax;
					j++;
				} else
				{
					//j & k пересекаются
					flag=1;
				}
			} else
			{
				temp.push_back(BoundRange());
				temp.back().xmin=b[k].xmin;
				if(b[k].xmax<a[j].xmin)
				{
					temp.back().xmax=b[k].xmax;
					k++;
				} else
				{
					//j & k пересекаются
					flag=1;
				}
			}
		}
	}

	if(j<a_n)
	{
		while(j<a_n)
		{
			temp.push_back(a[j]);
			j++;
		}
	}
	if(k<b_n)
	{
		while(k<b_n)
		{
			temp.push_back(b[k]);
			k++;
		}
	}

	CopyLine(ii,temp);
}

Bound& Bound::operator |=(Bound& bnd)
{
//	if(bnd.dy==0)return *this;
	int a_ymin=basey,a_ymax=basey+dy;
	int b_ymin=bnd.basey,b_ymax=bnd.basey+bnd.dy;

	int i;
	int yi=max(a_ymin,b_ymin);
	int ya=min(a_ymax,b_ymax);

	int yii=min(a_ymin,b_ymin);
	int yaa=max(a_ymax,b_ymax);


	if(yii<a_ymin || yaa>a_ymax)
	{//Маловато
		VERIFY(Realloc(yii,yaa-yii));
	}

	if(a_ymin>b_ymin)
	{
		//Значит b_ymin==yii
		int max=min(yi,b_ymax);
		for(i=yii;i<max;i++)
		{
			CopyLine(i-basey,bnd,i-bnd.basey);
		}
	}
	
	if(a_ymax<b_ymax)
	{
		//Значит b_ymax==yaa
		int min=max(ya,b_ymin);
		for(i=min;i<yaa;i++)
		{
 			CopyLine(i-basey,bnd,i-bnd.basey);
		}
	}

	//Случай не пересекающихся по y областей
	if(a_ymin>b_ymax || a_ymax<b_ymin)
	{
		int imin,imax;
		if(a_ymin>b_ymax)
		{
			imin=b_ymax;
			imax=a_ymin;
		}else
		{
			imin=a_ymax;
			imax=b_ymin;
		}
		imin-=basey;
		imax-=basey;
		xassert(imin>=0 && imin<dy);
		xassert(imax>=0 && imax<dy);
		for(i=imin;i<imax;i++) yrange[i].clear();
	}

	for(i=yi;i<ya;i++)
	{
		int i1,i2;
		i1=i-basey;
		i2=i-bnd.basey;
		xassert(i1>=0 && i1<dy);
		xassert(i2>=0 && i2<bnd.dy);

		OnLineOr(i1,bnd.yrange[i2]);
	}

//	ymin=yii;
//	ymax=yaa;

	return *this;
}


void Bound::OnLineAnd(int ii,vBoundRange& b)
{
	int j,k;

	vBoundRange& a=yrange[ii];
	int a_n=a.size();
	int b_n=b.size();

	temp.clear();
	j=k=0;

	int flag=0;
	int xi;
	while( j<a_n && k<b_n)
	{
		if(flag)
		{
			if(a[j].xmax<b[k].xmax)
				xi=a[j++].xmax;
			 else xi=b[k++].xmax;

			temp.back().xmax=xi;
			flag=0;
		} else
		{
			if(a[j].xmin<b[k].xmin)
			{
				if(a[j].xmax<b[k].xmin) j++; 
				 else
				 {
					 temp.push_back(BoundRange());
					 temp.back().xmin=b[k].xmin;
					 flag=1;
				 }
			} else
			{
				if(b[k].xmax<a[j].xmin) k++;
				 else 
				 {
					 temp.push_back(BoundRange());
					 temp.back().xmin=a[j].xmin;
					 flag=1;
				 }
			}
		}
	}

	CopyLine(ii,temp);
}

Bound& Bound::operator &=(Bound& bnd)
{
	if(bnd.dy==0)return *this;
	int a_ymin=basey,a_ymax=basey+dy;
	int b_ymin=bnd.basey,b_ymax=bnd.basey+bnd.dy;

	int i;
	int yi=max(a_ymin,b_ymin);
	int ya=min(a_ymax,b_ymax);
	if(yi>ya)yi=ya;

	int yii=min(a_ymin,b_ymin);
	int yaa=max(a_ymax,b_ymax);
	//Остальное затереть
	for(i=basey;i<yi;i++)
		yrange[i-basey].clear();
	for(i=ya;i<a_ymax;i++)
		yrange[i-basey].clear();
        //
	for(i=yi;i<ya;i++)
	{
		int i1,i2;
		i1=i-basey;
		i2=i-bnd.basey;
		OnLineAnd(i1,bnd.yrange[i2]);
	}

	return *this;
}
void Bound::Clear()
{
	for(int i=0;i<dy;i++) yrange[i].clear();
}
bool Bound::IsEmpty()
{
	for(int i=0;i<dy;i++)
	{
		if(!yrange[i].empty()) return false;
	}
	return true;
}
Bound& Bound::operator =(Bound& bnd)
{
	if(basey!=bnd.basey || dy!=bnd.dy)
	{
		Delete();
		basey=bnd.basey;
		dy=bnd.dy;
		New();
	}
	
	for(int y=0;y<dy;y++)
		yrange[y]=bnd.yrange[y];

	return *this;
}


Bound* Bound::Not(RECT r)
{
	int i;
	
	Bound* b=new Bound(r.top,r.bottom-r.top);
	b->Clear();

	xassert(basey>=r.top);
	xassert(basey+dy<=r.bottom);

	int mx=basey-r.top;
	for(i=0;i<mx;i++)
	{
		vBoundRange& cur=b->yrange[i];
		cur.push_back(BoundRange());
		cur.back().xmin=r.left;
		cur.back().xmax=r.right;
	}


	for(i=0;i<dy;i++)
	{
		if(yrange[i].empty())
		{
			int ii=i+basey-b->basey;
			vBoundRange& cur=b->yrange[ii];
			cur.push_back(BoundRange());
			cur.back().xmin=r.left;
			cur.back().xmax=r.right;
		}else
		{
			vBoundRange& a=yrange[i];
			int a_n=a.size();

			temp.clear();
			xassert(r.left<=a[0].xmin);
			xassert(r.right>=a[a_n-1].xmax);
			
			int j=0;

			if(a[0].xmin>r.left)
			{
				temp.push_back(BoundRange());
				temp.back().xmin=r.left;
			}else 
			{
				temp.push_back(BoundRange());
				temp.back().xmin=a[j++].xmax;
			}

			while(j<a_n)
			{
				temp.back().xmax=a[j].xmin;
				temp.push_back(BoundRange());
				temp.back().xmin=a[j++].xmax;
			}

			if(a[a_n-1].xmax<r.right)
			{
				temp.back().xmax=r.right;
			}

			b->CopyLine(i+basey-b->basey,temp);
		}
	}

	mx=r.bottom-r.top;
	for(i=dy+basey-b->basey;i<mx;i++)
	{
		vBoundRange& cur=b->yrange[i];
		cur.push_back(BoundRange());
		cur.back().xmin=r.left;
		cur.back().xmax=r.right;
	}

	return b;
}

RECT Bound::CalcRect()
{
	int _xmin=0xFFFFFF,_xmax=-0xFFFFFF;
	for(int i=0;i<dy;i++)
	{
		vBoundRange& a=yrange[i];
		int a_n=a.size();

		for(int j=0;j<a_n;j++)
		{
			int xi=a[j].xmin;
			int xa=a[j].xmax;

			if(_xmin>xi)_xmin=xi;
			if(_xmax<xa)_xmax=xa;
		}
	}

	RECT r;
	r.left=_xmin; 
	r.right=_xmax;
	//Find MinY
	for(i=0;i<dy;i++)
	{
		vBoundRange& a=yrange[i];
		if(!a.empty())break;
	}
	r.top=basey+i;

	//Find MaxY
	for(i=dy-1;i>=0;i--)
	{
		vBoundRange& a=yrange[i];
		if(!a.empty())break;
	}
	r.bottom=basey+i+1;

	return r;
}

void Bound::OnLineClip(int i,
				  int r_left,int r_right
				  )
{
	vBoundRange& a=yrange[i];
	int a_n=a.size();

	temp.clear();

	int j=0;
	int xcur=0;
	for(;j<a_n;j++)
	{
		if(a[j].xmax>=r_left)break;
	}

	if(j<a_n)
	{
		BoundRange br={max(a[j].xmin,r_left),min(a[j].xmax,r_right)};
		if(br.xmin<=br.xmax)
			temp.push_back(br);
		j++;
	}

	for(;j<a_n;j++)
	{
		if(a[j].xmin>=r_right)break;
		temp.push_back(BoundRange());
		temp.back().xmin=a[j].xmin;
		if(a[j].xmax<=r_right)
		{
			temp.back().xmax=a[j].xmax;
		}else
		{
			temp.back().xmax=r_right;
			break;
		}
	}

	CopyLine(i,temp);
}

void Bound::Clip(RECT r)
{
	for(int i=0;i<dy;i++)
	{
		if(i+basey<r.top || i+basey>=r.bottom)
		{
			yrange[i].clear();
			continue;
		}
		OnLineClip(i,r.left,r.right);
	}
}

void Bound::Move(int move_x,int move_y)
{
	basey+=move_y;
	if(move_x==0)return;

	for(int i=0;i<dy;i++)
	{
		vBoundRange& a=yrange[i];
		int a_n=a.size();
		for(int j=0;j<a_n;j++)
		{
			a[j].xmin+=move_x;
			a[j].xmax+=move_x;
		}
	}
}

void Bound::OnLineOrAdd(int ii,vBoundRange& b,int move_x)
{
	int j,k;
	vBoundRange& a=yrange[ii];
	int a_n=a.size();
	int b_n=b.size();

	temp.clear();

	j=k=0;

	int flag=0;
	int xa;
	while( j<a_n && k<b_n)
	{
		if(flag)
		{
			if(a[j].xmax>b[k].xmax+move_x)
			{
				xa=a[j].xmax;
				while(b[k].xmax+move_x<=xa && k<b_n)k++;
				if(k<b_n)
				  if(b[k].xmin+move_x<=xa)
				    continue;
				j++;
			} else
			{
				xa=b[k].xmax+move_x;
				while(a[j].xmax<=xa && j<a_n) j++;
				if(j<a_n)
				  if(a[j].xmin<=xa)
				    continue;
				k++;
			}

			temp.back().xmax=xa;
			flag=0;
		} else
		{
			if(a[j].xmin<b[k].xmin+move_x)
			{
				temp.push_back(BoundRange());
				temp.back().xmin=a[j].xmin;
				if(a[j].xmax<b[k].xmin+move_x)
				{
					temp.back().xmax=a[j].xmax;
					j++;
				} else
				{
					//j & k пересекаются
					flag=1;
				}
			} else
			{
				temp.push_back(BoundRange());
				temp.back().xmin=b[k].xmin+move_x;
				if(b[k].xmax+move_x<a[j].xmin)
				{
					temp.back().xmax=b[k].xmax+move_x;
					k++;
				} else
				{
					//j & k пересекаются
					flag=1;
				}
			}
		}
	}

	if(j<a_n)
	{
		while(j<a_n)
		{
			temp.push_back(a[j]);
			j++;
		}
	}
	if(k<b_n)
	{
		while(k<b_n)
		{
			BoundRange br={b[k].xmin+move_x,b[k].xmax+move_x};
			temp.push_back(br);
			k++;
		}
	}

	CopyLine(ii,temp);
}

void Bound::CopyLinePlus(int i,//Куда копировать
		Bound& bnd,int j,//Откуда копировать
		int move_x
		)
{
	vBoundRange& b=bnd.yrange[j];
	int b_n=b.size();

	xassert(i>=0 && i<dy);
	vBoundRange& a=yrange[i];
	a.resize(b_n);

	for(int i=0;i<b_n;i++)
	{
		a[i].xmin=b[i].xmin+move_x;
		a[i].xmax=b[i].xmax+move_x;
	}

}
Bound& Bound::OrMove(Bound& bnd,int move_x,int move_y,RECT r)
{
	int a_ymin=basey,a_ymax=basey+dy;
	int b_ymin=bnd.basey+move_y,b_ymax=bnd.basey+move_y+bnd.dy;
	//Обрезаем b
	b_ymin=max(b_ymin,r.top);
	b_ymax=min(b_ymax,r.bottom);

	b_ymin=min(b_ymin,r.bottom);
	b_ymax=max(b_ymax,b_ymin);

	int i;
	int yi=max(a_ymin,b_ymin);
	int ya=min(a_ymax,b_ymax);

	int yii=min(a_ymin,b_ymin);
	int yaa=max(a_ymax,b_ymax);


	if(yii<a_ymin || yaa>a_ymax)
	{
		VERIFY(Realloc(yii,yaa-yii));
	}

	if(a_ymin>b_ymin)
	{
		//Значит b_ymin==yii
		int max=min(yi,b_ymax);
		for(i=yii;i<max;i++)
		{
			CopyLinePlus(i-basey,bnd,i-(bnd.basey+move_y),move_x);
			OnLineClip(i-basey,r.left,r.right);
		}
	}
	
	if(a_ymax<b_ymax)
	{
		//Значит b_ymax==yaa
		int min=max(ya,b_ymin);
		for(i=min;i<yaa;i++)
		{
 			CopyLinePlus(i-basey,bnd,i-(bnd.basey+move_y),move_x);
			OnLineClip(i-basey,r.left,r.right);
		}
	}

	//Случай не пересекающихся по y областей
	if(a_ymin>b_ymax || a_ymax<b_ymin)
	{
		int imin,imax;
		if(a_ymin>b_ymax)
		{
			imin=b_ymax;
			imax=a_ymin;
		}else
		{
			imin=a_ymax;
			imax=b_ymin;
		}
		imin-=basey;
		imax-=basey;
		xassert(imin>=0 && imin<dy);
		xassert(imax>=0 && imax<dy);
		for(i=imin;i<imax;i++) yrange[i].clear();
	}

	for(i=yi;i<ya;i++)
	{
		int i1,i2;
		i1=i-basey;
		i2=i-(bnd.basey+move_y);
		OnLineOrAdd(i1,bnd.yrange[i2],move_x);
		OnLineClip(i1,r.left,r.right);
	}

	return *this;
}


bool Bound::IsIn(POINT p)
{
	int y=p.y-basey;
	int x=p.x;
	if(y<0 || y>=dy)return false;

	vBoundRange& a=yrange[y];
	int a_n=a.size();

	for(int i=0;i<a_n;i++)
	{
		if(x>=a[i].xmin && x<a[i].xmax)
		{
			return true;
		}
	}
	return false;
}
/*
void Bound::Copy(ScrBitmap& b,int x,int y)
{
	BYTE* p=b.p;

	basey=y;
	if(dy<b.y)
	{
		NewSize(0,b.y,heavy);
		Clear();
	}

	for(int i=0;i<b.y;i++)
	{
		int xmin=x;
		int t_n=0;
		int t_xmin[MaxTemp];
		int t_xmax[MaxTemp];

		BYTE fl;
		int Max;
		while(1)
		{
			fl=*p++;
			if(fl==0)break;
			Max=*(WORD*)p;
			p+=2;

			if(fl==2)
			{
				xmin+=Max;
			}
			else
			{
				t_xmin[t_n]=xmin;
				xmin+=Max;
				t_xmax[t_n]=xmin;
				t_n++;

				p+=Max;
			}
		}

		CopyLine(i,t_n,t_xmin,t_xmax);
	}

}
*/
void Bound::SetRect(int x1,int y1,int x2,int y2)
{
	xassert(x1<=x2);
	xassert(y1<=y2);
	int dy=y2-y1;
	NewSize(y1,dy);

	for(int i=dy-1;i>=0;i--)
	{
		BoundRange br={x1,x2};
		yrange[i].push_back(br);
	}
}

#include <math.h>
void Bound::SetCircle(int x0,int y0,int r)
{
	xassert(r>0);

	NewSize(y0-r,2*r);

	for(int i=-r;i<r;i++)
	{
		int ii=i+r;
		int z=round(sqrtf(r*r-i*i));
		BoundRange br={x0-z,x0+z};
		yrange[ii].push_back(br);
	}
}

void Bound::SetEllipse(int x1,int y1,int x2,int y2)
{
	xassert(x2>x1);
	xassert(y2>y1);
	NewSize(y1,y2-y1);

	double x0=(x1+x2)/2.0;
	double y0=(y1+y2)/2.0;
	double dx=x2-x0;
	double dy=y2-y0;

	for(int i=y1;i<y2;i++)
	{
		int ii=i-y1;
		double y=(i-y0)/dy;
		double z=dx*sqrt(1-y*y);
		BoundRange br={(int)(x0-z),(int)(x0+z)};
		yrange[ii].push_back(br);
	}
}

void ShowBound(BYTE* pg,int mx,int my,Bound& bnd,BYTE color)
{
	int xi,xa;
	for(int i=0;i<bnd.dy;i++)
	{
		Bound::vBoundRange& b=bnd.yrange[i];
		int n=b.size();
		BYTE* p=pg+mx*(i+bnd.basey);
		for(int j=0;j<n;j++)
		{
			xi=b[j].xmin;
			xa=b[j].xmax;
			xassert(xi>=0);
			xassert(xa<=640);
			xassert(xi<=xa);
			memset(p+xi,color,xa-xi);
		}
	}

}

void AbsoluteShowBound(BYTE* pg,int x,int y,int mx,int my,Bound& bnd,BYTE color)
{
	if(bnd.basey>y+my || bnd.basey+bnd.dy<=y)return;

	int imin=max(bnd.basey,y);
	int imax=min(bnd.dy+bnd.basey,y+my);

	for(int i=imin;i<imax;i++)
	{
		Bound::vBoundRange& b=bnd.yrange[i-bnd.basey];
		int n=b.size();
		BYTE* p=pg+mx*(i-y);
		for(int j=0;j<n;j++)
		{
			int xi=b[j].xmin-x;
			int xa=b[j].xmax-x;
			if(xi<0)xi=0;
			if(xa>mx)xa=mx;
			if(xi<xa)memset(p+xi,color,xa-xi);
		}
	}
}
/*
extern "C" void memadd(void* out,BYTE b,int count);
void AbsoluteShowBoundAdd(BYTE* pg,int x,int y,int mx,int my,Bound& b,BYTE color)
{
	if(b.basey>y+my || b.basey+b.dy<=y)return;

	int imin=max(b.basey,y);
	int imax=min(b.dy+b.basey,y+my);

	for(int i=imin;i<imax;i++)
	{
		int* xmin=b.xmin+(i-b.basey)*b.heavy;
		int* xmax=b.xmax+(i-b.basey)*b.heavy;
		int n=b.n[i-b.basey];
		BYTE* p=pg+mx*(i-y);
		for(int j=0;j<n;j++)
		{
			int xi=xmin[j]-x;
			int xa=xmax[j]-x;
			if(xi<0)xi=0;
			if(xa>mx)xa=mx;
			if(xi<xa)
				memadd(p+xi,color,xa-xi);
		}
	}
}
*/

Bound* CreateRect(int x1,int y1,int x2,int y2)
{
	Bound* bnd=new Bound;
	bnd->SetRect(x1,y1,x2,y2);
	return bnd;
}

Bound* CreateCirc(int x0,int y0,int r)
{
	Bound* bnd=new Bound;
	bnd->SetCircle(x0,y0,r);
	return bnd;
}

void Bound::AddRect(const RECT& rc)
{
	xassert(rc.top>=basey);
	xassert(rc.bottom<=basey+dy);

	vBoundRange b(1);

	for(int i=rc.top;i<rc.bottom;i++)
	{
		int i1;
		i1=i-basey;
		b[0].xmin=rc.left;
		b[0].xmax=rc.right;
		OnLineOr(i1,b);
	}
}

void Bound::Print(FILE* f)
{
	fprintf(f,"basey=%2i , dy=%2i\n",basey,dy);

	for(int i=0;i<dy;i++)
	{
		vBoundRange& a=yrange[i];
		int nmax=a.size();
		fprintf(f,"i=%3i,y=%3i, n=%2i ",i,i+basey,nmax);

		for(int nx=0;nx<nmax;nx++)
		{
			fprintf(f,"(%3i,%3i) ",a[nx].xmin,a[nx].xmax);
		}

		fprintf(f,"\n");
	}
}


template <class T>
void InternalCreateByNullColor(T* lpSurface,DWORD lPitch,
							   DWORD dwWidth,DWORD dwHeight,Bound* b,T null)
{
	for(DWORD y=0;y<dwHeight;y++)
	{
		T* px=(T*)(lPitch*y+(BYTE*)lpSurface);

		int n=0;
		DWORD x=0;

		array<int> xmin,xmax;
		while(x<dwWidth)
		{
			int xi,xa;
			xi=x;
			while(x<dwWidth && !(*px==null))
			{
				x++;
				px++;
			}
			xa=x;
			if(xa>xi)
			{
				xmin.add(xi);
				xmax.add(xa);
			}
			
			while(x<dwWidth && *px==null)
			{
				x++;
				px++;
			}
			
		}

		if(xmin.GetSize())
			b->CopyLine(y,xmin.GetSize(),&xmin[0],&xmax[0]);
		else
			b->CopyLine(y,0,NULL,NULL);

	}
}
