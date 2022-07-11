#ifndef __CIRCLE_MANAGER_PARAM_H__
#define __CIRCLE_MANAGER_PARAM_H__
#include "..\Render\inc\Umath.h"

struct CircleManagerParam
{
	float length;
	float width;
	float segment_len;
	bool useLegionColor;
	sColor4c diffuse;
	string texture_name; 
	CircleManagerParam();

	void serialize(Archive& ar);
};

enum CIRCLE_MANAGER_DRAW_ORDER
{
	CIRCLE_MANAGER_DRAW_BEFORE_GRASS_NOZ=0,
	CIRCLE_MANAGER_DRAW_AFTER_GRASS_NOZ=1,
	CIRCLE_MANAGER_DRAW_NORMAL_ALPHA=2,
};


struct SelectionBorderColor
{
	sColor4f selectionBorderColor_;
	string selectionTextureName_;
	string selectionCornerTextureName_;
	string selectionCenterTextureName_;
	bool needSelectionCenter_;

	SelectionBorderColor();
	void serialize(Archive& ar);
};

struct CircleColor {
	sColor4c color;
	float width;
	float length;

	enum CircleType{ 
		CIRCLE_SOLID = 0,
		CIRCLE_CHAIN_LINE,
		CIRCLE_TRIANGLE,
		CIRCLE_CROSS
	};
	CircleType dotted;

	CircleColor(sColor4c _color=sColor4c(255, 255, 255, 255), float _width=1.f, float _length=30.f, CircleType _dotted = CIRCLE_SOLID) {
		color=_color;
		width=_width;
		length=_length;
		dotted=_dotted;
	}
	void serialize(Archive& ar);
};

enum CircleUniqueID{
	RADIUS_UNIT_SIGHT = -1,
	RADIUS_UNIT_FIRE = -2,
	RADIUS_UNIT_MIN_FIRE = -3,
	RADIUS_PLACEMENT_ZONE = -4,
	RADIUS_SQUAD_AUTOJOINE = -5,
	RADIUS_SQUAD_JOINE = -6
};

class CircleEffect : public CircleColor
{
public:
	enum GraphType{
		EFFECT,
		CIRCLE,
		BOTH
	};

	CircleEffect() : type_(CIRCLE) {}
	GraphType type() const { return type_;	}

	void serialize(Archive& ar);

private:
	GraphType type_;
};

#endif //__CIRCLE_MANAGER_PARAM_H__
