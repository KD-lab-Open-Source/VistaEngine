#include "StdAfx.h"
#include "Serialization.h"
#include "..\Render\inc\uMath.h"
#include "ComboListColor.h"

void ComboListColor::serialize (Archive& ar)
{
    if (ar.isEdit ()) {
        ar.serialize(value_, "color", "Цвет");
        ar.serialize(comboList_, "comboList_", 0);
    } else {
		int idx = index();
		ar.serialize(idx, "index", 0);
		setIndex(idx);
    }
}

int ComboListColor::index() const
{
	ColorContainer::const_iterator i = find(comboList().begin(), comboList().end(), value_);
	return i != comboList().end() ? i - comboList().begin() : 0;
}

void ComboListColor::setIndex(int index)
{
	xassert(index >= 0 && index < comboList().size());
	value_ = comboList()[index];
}
