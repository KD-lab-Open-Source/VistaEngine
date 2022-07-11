#ifndef __SAVER_RENDER_H_INCLUDED__
#define __SAVER_RENDER_H_INCLUDED__

inline
Saver& operator<<(Saver& s,const sPolygon& p)
{
	s<<p.p1<<p.p2<<p.p3;
	return s;
}

inline
void operator>>(CLoadIterator& it,sPolygon& p)
{
	it>>p.p1;
	it>>p.p2;
	it>>p.p3;
}

inline
Saver& operator<<(Saver& s,const sRectangle4f& p)
{
	s<<p.min;
	s<<p.max;
	return s;
}

inline
void operator>>(CLoadIterator& it,sRectangle4f& p)
{
	it>>p.min;
	it>>p.max;
}


#endif
