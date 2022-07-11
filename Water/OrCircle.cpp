#include "StdAfx.h"
#include "OrCircle.h"
#include "Render\inc\IRenderDevice.h"

//О пересечении интервалов на круге.

//Если разбить круг на 4 четвертинки, то не будет проблеммы с пересечением.
//А если на 8 частей - то не будет проблеммы с точностью.

//Либо на 4 части, но под 45 градусов.!
//Каждая точка отностится к одной из 4 частей.
//На каждом из интервалов можно сказать - справа или слева точка.
//Но так как круг круглый, то нужно выбор интервала, чтобы потом пересекать его с другим интервалом.

//Нужна функция - точка на интервале или вне.
int num_circle=0;

int calcInterval(const Vect2f& p)
{
	int interval;
	if(fabsf(p.x) > fabsf(p.y))
		interval=p.x>0?0:2;
	else
		interval=p.y>0?1:3;
	return interval;
}

bool biggerCircleOneIntervalInternal(const Vect2f& A,const Vect2f& B)//Должны быть в одной четверти (против часовой стрелки)
{
	if(fabsf(A.x) > fabsf(A.y))
	{
//		xassert(A.y!=B.y);
		if(A.x>0)
			return A.y>=B.y;
		else
			return A.y<B.y;
	}
	else{
//		xassert(A.x!=B.x);
		if(A.y>0)
			return A.x<B.x;
		else
			return A.x>=B.x;
	}
}

bool biggerCircleOneIntervalEq(const Vect2f& A,const Vect2f& B)
{
//	xassert(fabsf(A.x)!=fabsf(A.y));
	if(A.x==B.x && A.y==B.y)
		return true;
	return biggerCircleOneIntervalInternal(A,B);
}

bool biggerCircleOneInterval(const Vect2f& A,const Vect2f& B)
{
//	xassert(fabsf(A.x)!=fabsf(A.y));
	if(A.x==B.x && A.y==B.y)
		return false;
	return biggerCircleOneIntervalInternal(A,B);
}

enum E_INTERSECT
{
	INTERSECT_NO=0,
	INTERSECT_YES=1,
	INTERSECT_BOTRER_A=2,
	INTERSECT_BOTRER_B=3,
};

E_INTERSECT intrecect(const Vect2f& A,const Vect2f& B,const Vect2f& m)
{/*
	if(A.x==m.x && A.y==m.y)
		return INTERSECT_BOTRER_A;
	if(B.x==m.x && B.y==m.y)
		return INTERSECT_BOTRER_B;
*/
	int ia=calcInterval(A);
	int ib=calcInterval(B);
	int im=calcInterval(m);

	if(ib==ia)
	{
		if(im==ia)
		{
			if(biggerCircleOneInterval(A,B))
			{
				bool ba=biggerCircleOneIntervalEq(m,A);
				bool nbb=biggerCircleOneInterval(B,m);
				if(ba || nbb)
				{
					return INTERSECT_YES;
				}
			}else
			{
				bool ba=biggerCircleOneIntervalEq(m,A);
				bool nbb=biggerCircleOneInterval(B,m);
				if(ba && nbb)
				{
					return INTERSECT_YES;
				}
			}
		}else
		{
			if(biggerCircleOneInterval(A,B))
				return INTERSECT_YES;
			else
				return INTERSECT_NO;
		}
		return INTERSECT_NO;
	}

	if(im==ia)
	{
		if(biggerCircleOneIntervalEq(m,A))
		{
			return INTERSECT_YES;
		}
		return INTERSECT_NO;
	}

	if(im==ib)
	{
		if(biggerCircleOneInterval(B,m))
		{
			return INTERSECT_YES;
		}
		return INTERSECT_NO;
	}

	int ibmax=ib>ia?ib:(ib+4);
	for(int i=ia+1;i<ibmax;i++)
	{
		int ii=i;
		if(ii>=4)
			ii-=4;

		if(ii==im)
		{
			return INTERSECT_YES;
		}
	}

	return INTERSECT_NO;
}

//Возвращает количество интервалов.
int clipCircle(OrCircle::Segment& s0,OrCircle::Segment& s1)
 {
	E_INTERSECT a1=intrecect(s0.A,s0.B,s1.A);
	E_INTERSECT b1=intrecect(s0.A,s0.B,s1.B);

	if(a1 && b1)
	{
		E_INTERSECT a0=intrecect(s1.A,s1.B,s0.A);
		E_INTERSECT b0=intrecect(s1.A,s1.B,s0.B);

		//Тут нужно определить - один или два интервала получилось.
		if(a0)
		{
//			xassert(b0);
			swap(s0.pB,s1.pB);swap(s0.B,s1.B);
			if(a0==INTERSECT_BOTRER_A)
				return 1;
			return 2;
		}else
		{
			swap(s0.pA,s1.pA);swap(s0.A,s1.A);
			swap(s0.pB,s1.pB);swap(s0.B,s1.B);
			return 1;
		}

	}

	if(a1) //&&!b1
	{
		swap(s0.pA,s1.pA);swap(s0.A,s1.A);
		return 1;
	}

	if(b1) //&&!a1
	{
		swap(s0.pB,s1.pB);swap(s0.B,s1.B);
		return 1;
	}

	{
		E_INTERSECT a0=intrecect(s1.A,s1.B,s0.A);
		E_INTERSECT b0=intrecect(s1.A,s1.B,s0.B);
		if(a0)
		{//Очень частый случай теоретически должен быть.
			return 1;
		}
	}

	return 0;
}

//(x-x0)^2+(y-y0)^2=r0^2
//(x-x1)^2+(y-y1)^2=r1^2

//x^2+y^2=r0^2
//(x-x1)^2+y^2=r1^2

//(x-x1)^2+r0^2-x^2=r1^2
//-2*x*x1=r1^2-x1^2-r0^2
//x=(r0^2+x1^2-r1^2)/(2*x1)
//y=+-sqrt(r0^2-x^2)

//X=(x1-x0,y1-y0)
//Y=(-X.y,X.x)
//XY0=(x0,y0)


INTERSECT_CIRCLE IntersectCircle(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1,
					 Vect2f& A0,Vect2f& B0,Vect2f& A1,Vect2f& B1)
{
	float dist=pos1.distance(pos0);
	float eps=1e-3f;
	if(dist<eps)
	{
		if(fabsf(r0-r1)<eps)
			return IC_0_EQUAL_1;
		return r0<r1?IC_0_IN_1:IC_1_IN_0;
	}

	float x=(r0*r0+dist*dist-r1*r1)/(2*dist);
	if(fabsf(x)>=r0)
	{
		if(r1+eps>r0+dist)
			return IC_0_IN_1;
		if(r0+eps>r1+dist)
			return IC_1_IN_0;
		return IC_NOTINTERSECT;
	}

	float y=sqrtf(r0*r0-x*x);
	Vect2f A,B;
	A.x=B.x=x;
	A.y=y;
	B.y=-y;
	Vect2f X=pos1-pos0;
	X.normalize(1.0f);
	Mat2f m0(X.x,-X.y,
		     X.y, X.x);

	A0=m0*A;
	B0=m0*B;


	A1=B0+pos0-pos1;
	B1=A0+pos0-pos1;
	return IC_INTERSECT;
}

void DrawCircle(const Vect2f& pos,float r,Color4c color=Color4c(255,255,255))
{
	int segments=round(r/3.0f);
	Vect2f cur,old;
	old=pos;
	old.x+=r;
	for(int i=1;i<=segments;i++)
	{
		float alpha=(2*M_PI*i)/segments;
		cur.x=r*cos(alpha)+pos.x;
		cur.y=r*sin(alpha)+pos.y;
		gb_RenderDevice->DrawLine(old.x,old.y,cur.x,cur.y,color);
		old=cur;
	}

}

void DrawSegment(const Vect2f& pos,float r,const Vect2f& A,const Vect2f& B,Color4c color,Color4c* pcolor2=0)
{
	float dalpha=3/r;
	float a0=atan2(A.y,A.x);
	float a1=atan2(B.y,B.x);
	if(a0>a1)
		a1+=2*M_PI;

	float alpha=a0;
	Vect2f cur,old;
	cur.x=r*cos(alpha)+pos.x;
	cur.y=r*sin(alpha)+pos.y;

	Color4c color2=pcolor2?*pcolor2:color;

	for(;alpha<a1;alpha+=dalpha)
	{
		old=cur;
		cur.x=r*cos(alpha)+pos.x;
		cur.y=r*sin(alpha)+pos.y;

		Color4c c;
		float t=(alpha-a0)/(a1-a0);
		xassert(t>=-1e-3f && t<1.001f);
		t=clamp(t,0.0f,0.999f);
		c.interpolate(color,color2,t);


		gb_RenderDevice->DrawLine(old.x,old.y,cur.x,cur.y,c);
	}

	old=cur;
	alpha=a1;
	cur.x=r*cos(alpha)+pos.x;
	cur.y=r*sin(alpha)+pos.y;
	gb_RenderDevice->DrawLine(old.x,old.y,cur.x,cur.y,color2);
}

void TestIntersect(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1)
{
	DrawCircle(pos0,r0);
	DrawCircle(pos1,r1);
	Vect2f A0,B0,A1,B1;
	if(!IntersectCircle(pos0,r0,pos1,r1,
					 A0,B0,A1,B1))
	{
		//DrawSegment(pos0,r0,A0,B0,Color4c(0,0,255));
		DrawSegment(pos1,r1,A1,B1,Color4c(0,0,255));
//		gb_RenderDevice->DrawRectangle(A0.x+pos0.x-1,A0.y+pos0.y-1,3,3,Color4c(255,0,0));
//		gb_RenderDevice->DrawRectangle(B0.x+pos0.x-1,B0.y+pos0.y-1,3,3,Color4c(0,255,0));
		gb_RenderDevice->DrawRectangle(A1.x+pos1.x-1,A1.y+pos1.y-1,3,3,Color4c(255,0,0));
		gb_RenderDevice->DrawRectangle(B1.x+pos1.x-1,B1.y+pos1.y-1,3,3,Color4c(0,255,0));
	}
	gb_RenderDevice->FlushPrimitive2D();
}
/*
void TestSegment(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1,const Vect2f& pos2,float r2)
{
	DrawCircle(pos0,r0);
	DrawCircle(pos1,r1);
	DrawCircle(pos2,r2);
	Vect2f A0,B0,A1,B1;
	Vect2f A20,B20,A21,B21;
	if(!IntersectCircle(pos0,r0,pos1,r1,
					 A0,B0,A1,B1))
	{
		if(!IntersectCircle(pos0,r0,pos2,r2,
						A20,B20,A21,B21))
		{
			OrCircle::Segment s0,s1;
			s0.A=A0;
			s0.B=B0;
			s1.A=A20;
			s1.B=B20;
			int segments=clipCircle(s0,s1);
			if(segments>=1)
				DrawSegment(pos0,r0,s0.A,s0.B,Color4c(0,0,255));
			if(segments>=2)
				DrawSegment(pos0,r0,s1.A,s1.B,Color4c(255,0,0));
		}
	}

	gb_RenderDevice->FlushPrimitive2D();
}
*/
void OrCircle::Circle::clear()
{
	vector<Segment*>::iterator it;
	FOR_EACH(segments,it)
	{
		delete *it;
	}
	segments.clear();
}

OrCircle::Circle::~Circle()
{
	clear();
}

void OrCircle::clear()
{
	vector<Circle*>::iterator it;
	FOR_EACH(circles,it)
	{
		delete *it;
	}

	circles.clear();
	tree.clear();
}

OrCircle::~OrCircle()
{
	clear();
}

void OrCircle::add(const Vect2f& pos,float r)
{
	Circle* c=new Circle;
	c->pos=pos;
	c->r=r;
	c->added=false;
	circles.push_back(c);
}

void OrCircle::calc()
{
	num_circle=0;

	tree.build(circles);
	vector<Circle*>::iterator it;
	FOR_EACH(circles,it)
	{
		Circle* c=*it;
		Segment* s=new_segment(c);
		c->segments.push_back(s);
		intersect(c);
		c->added=true;
	}
/*
	FOR_EACH(circles,it)
	{
		Circle& circle=**it;
		vector<Segment*>& segments=circle.segments;
		vector<Segment*>::iterator it;
		FOR_EACH(segments,it)
		{
			Segment& s=**it;
			if(s.type==SS_SEGMENT)
			{
				update_link(&s);
			}
		}
	}
*/
}

void OrCircle::testDraw()
{
	int color=0;
	Color4c colors[]=
	{
		Color4c(0,0,255),
		Color4c(0,255,0),
		Color4c(255,0,0),
		Color4c(255,255,0),
		Color4c(255,0,255),
		Color4c(255,255,0),
	};
	int num_colors=sizeof(colors)/sizeof(colors[0]);

	vector<Circle*>::iterator itc;
	FOR_EACH(circles,itc)
	{
		Circle& circle=**itc;
		vector<Segment*>& segments=circle.segments;
		vector<Segment*>::iterator it;
		FOR_EACH(segments,it)
		{
			Segment& s=**it;
			if(s.type==SS_SEGMENT)
			{
				s.color=colors[color];
				color=(color+1)%num_colors;
			}

			if(s.type==SS_ALL_CIRCLE)
			{
				s.color=Color4c(255,255,255);
			}
		}
	}

	FOR_EACH(circles,itc)
	{
		Circle& circle=**itc;
		vector<Segment*>& segments=circle.segments;
		vector<Segment*>::iterator it;
		FOR_EACH(segments,it)
		{
			Segment& s=**it;
			if(s.type==SS_SEGMENT)
			{
				DrawSegment(circle.pos,circle.r,s.A,s.B,colors[color],&Color4c(255,255,255));
				//DrawSegment(circle.pos,circle.r,s.A,s.B,s.color,&s.pB->color);
/*
				if(s.pA->s0)
					DrawSegment(circle.pos,circle.r,s.A(),s.B(),s.pA->s0->color,&s.color);
				else
					DrawSegment(circle.pos,circle.r,s.A(),s.B(),Color4c(0,0,0));
*/
				color=(color+1)%num_colors;
			}

			if(s.type==SS_ALL_CIRCLE)
			{
				DrawCircle(circle.pos,circle.r,Color4c(255,255,255));
			}
		}
	}
}


void CircleIntersectProc(OrCircle::Circle* obj,void* param)
{
	num_circle++;
	OrCircle* p=(OrCircle*)param;
	if(obj->added)
		p->intersect2(obj,p->cur_circle);
}

void OrCircle::intersect(Circle* circle)
{
	//Если круг внутри другого круга - убираем его.
	//Если круг вне другого круга - оставляем.
	//Иначе клипуем сегменты гругом и если получаются дополнительные сегменты - вставляем.
	//Если у круга не осталось сегментов - с ним можно не проверять пересечение (убираем круг).

	//Сначала написать за N^2 а потом уже оптимизировать по местоположению.

	//Поклиповать все сегменты гругом
	//Поклиповать сегменты этого круга остальными кругмаи.

	//Как клиповать сегмент кругом
	//Клипуем круг с кругом, а потом пересекаем сегменты.
	//Делаем фикс связности сегментов.
/*
	vector<Circle*>::iterator it;
	FOR_EACH(circles,it)
	{
		num_circle++;
		if((*it)->added)
			intersect2(*it,circle);
	}
/*/
	int xmin,ymin,xmax,ymax;
	tree.GetBorder(circle,xmin,ymin,xmax,ymax);
	cur_circle=circle;
	tree.find(xmin,ymin,xmax,ymax,CircleIntersectProc,this);
	int k=0;
/**/
}

OrCircle::Segment* OrCircle::new_segment(OrCircle::Circle* circle)
{
	Segment* p=new Segment;
	p->source=circle;
	return p;
}
/*
   Попробовать иной вариант связывания сегментов.
   1. Есть точки, каждая точка находится одновременно на 2 кругах.
   2. На точки есть ссылки с 2х сегментов
   3. Если на точки не осталось ссылок - они уничтожаются.
   4. Связность получается автоматически через точки.
*/

void OrCircle::intersect2(Circle* circle0,Circle* circle1)
{

	if(false)
	{//Получается, что даже круг без сегментов может вырезать сегмент в другом круге.
	 //Этот кусок конечно неправильный, но как тогда уменьшать область?
	 //Ответ - чисто посегментно.
	//То есть надо написать функцию определения bound у сегмента и пересекать его только с кругами
	//по дереву.
	//Хотя дерево должно быть хитрое, чтобы искать пересечение с постоянно уменьшающимся bound.
		if(circle0->segments.empty())
			return;
		if(circle1->segments.empty())
			return;
	}

	Vect2f A0,B0,A1,B1;
	INTERSECT_CIRCLE ic=IntersectCircle(circle0->pos,circle0->r,circle1->pos,circle1->r,
						 A0,B0,A1,B1);
	if(ic==IC_NOTINTERSECT)
		return;
	if(ic==IC_0_IN_1)
	{
		circle0->clear();
		return;
	}
	if(ic==IC_1_IN_0)
	{
		circle1->clear();
		return;
	}
	if(ic==IC_0_EQUAL_1)
	{
		if(!circle0->segments.empty() && circle0->segments[0]->type==SS_ALL_CIRCLE)
		{
			circle0->clear();
			return;
		}
		circle1->clear();
		return;
	}

	intersect_segment(circle0,A0,B0,circle1);
	intersect_segment(circle1,A1,B1,circle0);
}

void OrCircle::intersect_segment(Circle* circle,Vect2f& A,Vect2f& B,Circle* pIntersect)
{
	//A=pA->p0,B=pB->p1
	vector<Segment*> sold;
	sold.swap(circle->segments);
	circle->segments.clear();
	vector<Segment*>::iterator it;
	FOR_EACH(sold,it)
	{
		Segment* ps0=*it;
		Segment s1;
		s1.source=circle;
		s1.A=A;
		s1.B=B;
		s1.pA=pIntersect;
		s1.pB=pIntersect;
		s1.type=SS_SEGMENT;
		int num;
		if(ps0->type==SS_ALL_CIRCLE)
		{
			num=1;
			swap(ps0->pA,s1.pA);
			swap(ps0->pB,s1.pB);
			swap(ps0->A,s1.A);
			swap(ps0->B,s1.B);
		}else
		{
			num=clipCircle(*ps0,s1);
		}
		if(num==0)
		{
			delete ps0;
		}

		if(num>=1)
		{
			ps0->type=SS_SEGMENT;
			circle->segments.push_back(ps0);
		}

		if(num==2)
		{
			Segment* ps1=new Segment;
			*ps1=s1;
			circle->segments.push_back(ps1);
		}
	}
}

OrCircle::QuatTreeCircle::QuatTreeCircle()
{
}

OrCircle::QuatTreeCircle::~QuatTreeCircle()
{
}

void OrCircle::QuatTreeCircle::clear()
{
	tree.clear();
}

void OrCircle::QuatTreeCircle::find(Vect2i pos,int radius,find_proc proc,void* param)
{
	find(pos.x-radius,pos.y-radius,pos.x+radius,pos.y+radius,proc,param);
}

void OrCircle::QuatTreeCircle::find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param)
{
	tree.find(xmin,ymin,xmax,ymax,(QuatTreeVoid::find_proc)proc,param);
}

void OrCircle::QuatTreeCircle::GetBorder(OrCircle::Circle* obj,int& xmin,int& ymin,int& xmax,int& ymax)
{
	xmin=(int)obj->pos.x-obj->r;
	xmax=(int)obj->pos.x+obj->r;
	ymin=round(obj->pos.y-obj->r+0.5f);
	ymax=round(obj->pos.y+obj->r+0.5f);
}

void OrCircle::QuatTreeCircle::build(vect& blist)
{
	clear();
	int sz=blist.size();
	if(!sz)
		return;
	vector<QuatTreeVoid::OneObj> tlist(sz);
	for(int i=0;i<sz;i++)
	{
		QuatTreeVoid::OneObj& o=tlist[i];
		GetBorder(blist[i],o.xmin,o.ymin,o.xmax,o.ymax);
		o.obj=blist[i];
	}

	tree.build(tlist);
}

void OrCircle::get_spline(OrCircleSplines& splines,float segment_len)
{
	OrCircleSpline vct;
	vector<Circle*>::iterator itc;
	FOR_EACH(circles,itc)
	{
		Circle& circle=**itc;
		vector<Segment*>& segments=circle.segments;
		vector<Segment*>::iterator it;
		FOR_EACH(segments,it)
		{
			Segment& s=**it;
			if(!s.in_spline)
			{
				splines.push_back(vct);
				BuildSpline(splines.back(),&s,segment_len);
			}
		}
	}

}

void OrCircle::BuildSpline(OrCircleSpline& spline,Segment* begin,float segment_len)
{
	Segment* cur=begin;
	while(!cur->in_spline)
	{
		BuildSegment(spline,cur,segment_len,false,30);
		cur->in_spline=true;
		if(cur->type==SS_ALL_CIRCLE)
			break;
		cur=cur->sB;
	}
}

void OrCircle::BuildSegment(OrCircleSpline& spline,Segment* segment,float segment_len,bool add_first,float length)
{
	float r=segment->source->r;
	Vect2f pos=segment->source->pos;
	float a0,a1;
	if(segment->type==SS_ALL_CIRCLE)
	{
		a0=0;
		a1=2*M_PI;
	}else
	{
		Vect2f& A=segment->A;
		Vect2f& B=segment->B;
		a0=atan2(A.y,A.x);
		a1=atan2(B.y,B.x);
	}


	if(a0>a1)
		a1+=2*M_PI;
	float inv_r=1/r;
	float dalpha=segment_len/r;
	if(dalpha>M_PI/6)
		dalpha=M_PI/6;

	float mul_t=r/length;

	float alpha=a0;
	OrCircleSplineElement cur;
	if(!add_first)
		alpha+=dalpha;

	float amax=a1-dalpha*0.5f;
	for(;alpha<a1;alpha+=dalpha)
	{
		cur.pos.x=r*cos(alpha)+pos.x;
		cur.pos.y=r*sin(alpha)+pos.y;
		cur.t=alpha*mul_t;
		cur.norm=cur.pos-pos;
		cur.norm*=inv_r;
		spline.push_back(cur);
	}

	alpha=a1;
	cur.pos.x=r*cos(alpha)+pos.x;
	cur.pos.y=r*sin(alpha)+pos.y;
	cur.t=alpha*mul_t;
	cur.norm=cur.pos-pos;
	cur.norm*=inv_r;
	spline.push_back(cur);
}

void OrCircle::get_circle_segment(OrCircleSplines& splines,float segment_len,float length)
{
	OrCircleSpline vct;
	vector<Circle*>::iterator itc;
	FOR_EACH(circles,itc)
	{
		Circle& circle=**itc;
		vector<Segment*>& segments=circle.segments;
		vector<Segment*>::iterator it;
		FOR_EACH(segments,it)
		{
			Segment& s=**it;
			splines.push_back(vct);
			BuildSegment(splines.back(),&s,segment_len,true,length);
		}
	}
}
