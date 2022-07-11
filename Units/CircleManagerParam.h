#ifndef __CIRCLE_MANAGER_PARAM_H__
#define __CIRCLE_MANAGER_PARAM_H__

#include "XMath\Colors.h"

struct CircleManagerParam
{
	float length;
	float width;
	float segmentLength;
	bool useLegionColor;
	Color4c color;
	string texture; 

	CircleManagerParam(const Color4c& colorIn = Color4c(255, 0, 0, 128));
	void serialize(Archive& ar);
};

enum CircleManagerDrawOrder
{
	CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ=0,
	CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ=1,
	CIRCLE_MANAGER_DRAW_NORMAL_ALPHA=2,
};


struct SelectionBorderColor
{
	Color4f selectionBorderColor_;
	string selectionTextureName_;
	string selectionCornerTextureName_;
	string selectionCenterTextureName_;
	bool needSelectionCenter_;

	SelectionBorderColor();
	void serialize(Archive& ar);
};

#endif //__CIRCLE_MANAGER_PARAM_H__
