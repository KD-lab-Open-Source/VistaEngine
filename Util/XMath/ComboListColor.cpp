#include "StdAfx.h"
Serialization/Serialization.h
#include "XMath/Colors.h"
#include "ComboListColor.h"

void ComboListColor::serialize (Archive& ar)
{
	ar.serialize(index_, "index", 0);
}

