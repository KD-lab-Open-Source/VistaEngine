#ifndef __FAKE_ATLTYPES_H__
#define __FAKE_ATLTYPES_H__

// Minimal cross-platform stand-ins for the MFC/ATL geometry helper classes
// CPoint, CSize and CRect. The originals derive from the Win32 POINT/SIZE/RECT
// structs (provided by Platform/WindowsAPI.h) and add convenience methods.
// Only the commonly used subset of the MFC API is implemented here.

#include <algorithm>

class CSize : public SIZE
{
public:
	CSize() { cx = cy = 0; }
	CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
	CSize(SIZE initSize) { *static_cast<SIZE*>(this) = initSize; }
	CSize(POINT initPt) { cx = initPt.x; cy = initPt.y; }

	bool operator==(SIZE size) const { return cx == size.cx && cy == size.cy; }
	bool operator!=(SIZE size) const { return cx != size.cx || cy != size.cy; }
	void operator+=(SIZE size) { cx += size.cx; cy += size.cy; }
	void operator-=(SIZE size) { cx -= size.cx; cy -= size.cy; }

	CSize operator+(SIZE size) const { return CSize(cx + size.cx, cy + size.cy); }
	CSize operator-(SIZE size) const { return CSize(cx - size.cx, cy - size.cy); }
	CSize operator-() const { return CSize(-cx, -cy); }
};

class CPoint : public POINT
{
public:
	CPoint() { x = y = 0; }
	CPoint(int initX, int initY) { x = initX; y = initY; }
	CPoint(POINT initPt) { *static_cast<POINT*>(this) = initPt; }
	CPoint(SIZE initSize) { x = initSize.cx; y = initSize.cy; }

	void Offset(int xOffset, int yOffset) { x += xOffset; y += yOffset; }
	void Offset(POINT point) { x += point.x; y += point.y; }
	void Offset(SIZE size) { x += size.cx; y += size.cy; }

	bool operator==(POINT point) const { return x == point.x && y == point.y; }
	bool operator!=(POINT point) const { return x != point.x || y != point.y; }
	void operator+=(SIZE size) { x += size.cx; y += size.cy; }
	void operator-=(SIZE size) { x -= size.cx; y -= size.cy; }
	void operator+=(POINT point) { x += point.x; y += point.y; }
	void operator-=(POINT point) { x -= point.x; y -= point.y; }

	CPoint operator+(SIZE size) const { return CPoint(x + size.cx, y + size.cy); }
	CPoint operator-(SIZE size) const { return CPoint(x - size.cx, y - size.cy); }
	CPoint operator+(POINT point) const { return CPoint(x + point.x, y + point.y); }
	CSize  operator-(POINT point) const { return CSize(x - point.x, y - point.y); }
	CPoint operator-() const { return CPoint(-x, -y); }
};

class CRect : public RECT
{
public:
	CRect() { left = top = right = bottom = 0; }
	CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	CRect(const RECT& srcRect) { *static_cast<RECT*>(this) = srcRect; }
	CRect(POINT topLeft, POINT bottomRight)
		{ left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }
	CRect(POINT point, SIZE size)
		{ left = point.x; top = point.y; right = left + size.cx; bottom = top + size.cy; }

	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	CSize Size() const { return CSize(right - left, bottom - top); }

	CPoint& TopLeft() { return *reinterpret_cast<CPoint*>(this); }
	const CPoint& TopLeft() const { return *reinterpret_cast<const CPoint*>(this); }
	CPoint& BottomRight() { return *(reinterpret_cast<CPoint*>(this) + 1); }
	const CPoint& BottomRight() const { return *(reinterpret_cast<const CPoint*>(this) + 1); }
	CPoint CenterPoint() const { return CPoint((left + right) / 2, (top + bottom) / 2); }

	bool IsRectEmpty() const { return left >= right || top >= bottom; }
	bool IsRectNull() const { return left == 0 && top == 0 && right == 0 && bottom == 0; }

	bool PtInRect(POINT point) const
		{ return point.x >= left && point.x < right && point.y >= top && point.y < bottom; }

	void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	void SetRectEmpty() { left = top = right = bottom = 0; }

	void OffsetRect(int x, int y) { left += x; right += x; top += y; bottom += y; }
	void OffsetRect(POINT pt) { OffsetRect(pt.x, pt.y); }
	void OffsetRect(SIZE size) { OffsetRect(size.cx, size.cy); }

	void InflateRect(int x, int y) { left -= x; top -= y; right += x; bottom += y; }
	void InflateRect(SIZE size) { InflateRect(size.cx, size.cy); }
	void DeflateRect(int x, int y) { left += x; top += y; right -= x; bottom -= y; }
	void DeflateRect(SIZE size) { DeflateRect(size.cx, size.cy); }

	void NormalizeRect()
	{
		if(left > right) std::swap(left, right);
		if(top > bottom) std::swap(top, bottom);
	}

	bool operator==(const RECT& rect) const
		{ return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }
	bool operator!=(const RECT& rect) const { return !(*this == rect); }

	void operator+=(POINT point) { OffsetRect(point); }
	void operator-=(POINT point) { OffsetRect(-point.x, -point.y); }

	CRect operator+(POINT point) const { CRect r(*this); r.OffsetRect(point); return r; }
	CRect operator-(POINT point) const { CRect r(*this); r.OffsetRect(-point.x, -point.y); return r; }

	operator RECT*() { return this; }
	operator const RECT*() const { return this; }
};

#endif // __FAKE_ATLTYPES_H__
