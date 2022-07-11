#ifndef __CD2D_H_INCLUDED__
#define __CD2D_H_INCLUDED__

bool penetrationCircleRectangle(float circleRadius, const Vect2f& circlePosition, 
								const Vect2f rectangleExtent, const Vect2f& rectanglePosition, const Mat2f& rectangleOrientation);

bool penetrationRectangleRectangle(const Vect2f& rectangle0Extent, const Vect2f& rectangle0Position, const Mat2f& rectangle0Orientation,
								   const Vect2f& rectangle1Extent, const Vect2f& rectangle1Position, const Mat2f& rectangle1Orientation);

#endif // __CD2D_H_INCLUDED__