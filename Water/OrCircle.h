#ifndef __OR_CIRCLE_H_INCLUDED__
#define __OR_CIRCLE_H_INCLUDED__

#include "XMath/xmath.h"
#include "XMath/Colors.h"
#include "XTL/Handle.h"
#include "Render\src\VisGrid2d.h"

enum INTERSECT_CIRCLE
{
	IC_INTERSECT=0,
	IC_NOTINTERSECT,
	IC_0_IN_1,
	IC_1_IN_0,
	IC_0_EQUAL_1,
};
INTERSECT_CIRCLE IntersectCircle(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1,
					 Vect2f& A0,Vect2f& B0,Vect2f& A1,Vect2f& B1);

void TestIntersect(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1);
void TestSegment(const Vect2f& pos0,float r0,const Vect2f& pos1,float r1,const Vect2f& pos2,float r2);

struct OrCircleSplineElement
{
	Vect2f pos;
	float t;
	Vect2f norm;
};

typedef vector<OrCircleSplineElement> OrCircleSpline;
typedef list<OrCircleSpline> OrCircleSplines;

class OrCircle : public ShareHandleBase
{
public:
	~OrCircle();
	void clear();
	void add(const Vect2f& pos,float r);
	void calc();
	void get_spline(OrCircleSplines& splines,float segment_len);
	void get_circle_segment(OrCircleSplines& splines,float segment_len,float length);

	void testDraw();

	enum SegmentState
	{
		SS_ALL_CIRCLE=0,
		SS_EMPTY,
		SS_SEGMENT,
	};

	struct Circle;
	struct Segment
	{	
		Circle* source;
		SegmentState type;
		Circle *pA,*pB;
		Vect2f A,B;
		Segment *sA,*sB;
		Color4c color;
		bool in_spline;

		Segment(){pA=pB=0;sA=sB=0;type=SS_ALL_CIRCLE;source=0;in_spline=false;}
	};

	struct Circle
	{
		Vect2f pos;
		float r;
		bool added;
		vector<Segment*> segments;

		void clear();
		~Circle();
	};

	class QuatTreeCircle
	{
		QuatTreeVoid tree;
	public:
		typedef vector<Circle*> vect;
		typedef void (*find_proc)(Circle* obj,void* param);

		QuatTreeCircle();
		~QuatTreeCircle();

		void find(Vect2i pos,int radius,find_proc proc,void* param);
		void find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param);

		void build(vect& blist);
		void clear();
		void GetBorder(Circle* p,int& xmin,int& ymin,int& xmax,int& ymax);
	};

protected:
	vector<Circle*> circles;
	QuatTreeCircle tree;
	Circle* cur_circle;

	void intersect(Circle* circle);
	Segment* new_segment(Circle* circle);

	void intersect2(Circle* circle1,Circle* circle2);
	void intersect_segment(Circle* circle,Vect2f& A,Vect2f& B,Circle* pCircle);
	void build_spline();

	void BuildSpline(OrCircleSpline& spline,Segment* begin,float segment_len);
	void BuildSegment(OrCircleSpline& spline,Segment* begin,float segment_len,bool add_first,float length);

	friend void CircleIntersectProc(Circle* obj,void* param);
};

#endif
