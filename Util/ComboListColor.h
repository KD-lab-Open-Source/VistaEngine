#ifndef __COMBO_LIST_COLOR_H_INCLUDED__
#define __COMBO_LIST_COLOR_H_INCLUDED__

#include <vector>
#include "..\Render\inc\uMath.h"

typedef std::vector<sColor4f> ColorContainer;

class ComboListColor
{
public:
    ComboListColor(const ColorContainer& comboList, const sColor4f& value) : comboList_(comboList), value_(value) {}
	ComboListColor() {
		value_ = sColor4f (0.0f, 0.0f, 0.0f);
	}
    ComboListColor& operator=(const sColor4f& value) { value_ = value; return *this; }

    operator const sColor4f& () const { return value_; }
    const ColorContainer& comboList() const { return comboList_; }
    void setComboList(const ColorContainer& comboList) { comboList_ = comboList; }

    sColor4f& value() { return value_; }
    const sColor4f& value() const { return value_; }

	int index() const;
	void setIndex(int index);

    void serialize (Archive& ar);

private:
    sColor4f value_;
    ColorContainer comboList_;
};

#endif
