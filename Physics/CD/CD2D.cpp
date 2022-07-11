#include "StdAfx.h"
#include "CD2D.h"

bool penetrationCircleRectangle(float circleRadius, const Vect2f& circlePosition, 
								const Vect2f rectangleExtent, const Vect2f& rectanglePosition, const Mat2f& rectangleOrientation)
{
	Vect2f dif(circlePosition);
	dif -= rectanglePosition;
	dif *= rectangleOrientation;
	dif.set(fabsf(dif.x), fabsf(dif.y));
	dif -= rectangleExtent;
	if(dif.x > circleRadius || dif.y > circleRadius || (dif.x > 0.0f && dif.y > 0.0f && dif.norm() > circleRadius))
		return false;
	return true;
}

bool penetrationRectangleRectangle(const Vect2f& rectangle0Extent, const Vect2f& rectangle0Position, const Mat2f& rectangle0Orientation,
								   const Vect2f& rectangle1Extent, const Vect2f& rectangle1Position, const Mat2f& rectangle1Orientation)
{
	Mat2f relativeOrientation(rectangle0Orientation);
	relativeOrientation.invert();
	Vect2f dif(rectangle1Position);
	dif -= rectangle0Position;
	dif *= relativeOrientation;
	relativeOrientation *= rectangle1Orientation;
	Mat2f relativeOrientationInv(relativeOrientation);
	relativeOrientationInv.invert();
	Vect2f difAbs(fabsf(dif.x), fabsf(dif.y));
	if(difAbs.x / rectangle0Extent.x > difAbs.y / rectangle0Extent.y){
		Vect2f extentTemp(relativeOrientationInv.xcol());
		extentTemp.set(extentTemp.x > 0 ? rectangle1Extent.x : -rectangle1Extent.x,
			extentTemp.y > 0 ? rectangle1Extent.y : -rectangle1Extent.y);
		extentTemp *= relativeOrientation;
		if(rectangle0Extent.x + fabsf(extentTemp.x) < difAbs.x)
			return false;
	} else {
		Vect2f extentTemp(relativeOrientationInv.ycol());
		extentTemp.set(extentTemp.x > 0 ? rectangle1Extent.x : -rectangle1Extent.x,
			extentTemp.y > 0 ? rectangle1Extent.y : -rectangle1Extent.y);
		extentTemp *= relativeOrientation;
		if(rectangle0Extent.y + fabsf(extentTemp.y) < difAbs.y)
			return false;
	}
	dif *= relativeOrientationInv;
	difAbs.set(fabsf(dif.x), fabsf(dif.y));
	if(difAbs.x / rectangle1Extent.x > difAbs.y / rectangle1Extent.y){
		Vect2f extentTemp(relativeOrientation.xcol());
		extentTemp.set(extentTemp.x > 0 ? rectangle0Extent.x : -rectangle0Extent.x,
			extentTemp.y > 0 ? rectangle0Extent.y : -rectangle0Extent.y);
		extentTemp *= relativeOrientationInv;
		if(rectangle1Extent.x + fabsf(extentTemp.x) < difAbs.x)
			return false;
	} else {
		Vect2f extentTemp(relativeOrientation.ycol());
		extentTemp.set(extentTemp.x > 0 ? rectangle0Extent.x : -rectangle0Extent.x,
			extentTemp.y > 0 ? rectangle0Extent.y : -rectangle0Extent.y);
		extentTemp *= relativeOrientationInv;
		if(rectangle1Extent.y + fabsf(extentTemp.y) < difAbs.y)
			return false;
	}
	return true;
}