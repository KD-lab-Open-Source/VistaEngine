#include "StdAfx.h"
#include "Saver.h"
#include "UMath.h"
#include "XMath\Rectangle4f.h"

Saver& operator<<(Saver& s,const sPolygon& p)
{
	s<<p.p1<<p.p2<<p.p3;
	return s;
}

void operator>>(CLoadIterator& it,sPolygon& p)
{
	it>>p.p1;
	it>>p.p2;
	it>>p.p3;
}

Saver& operator<<(Saver& s,const sRectangle4f& p)
{
	s<<p.min;
	s<<p.max;
	return s;
}

void operator>>(CLoadIterator& it,sRectangle4f& p)
{
	it>>p.min;
	it>>p.max;
}
