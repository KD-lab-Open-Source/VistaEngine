#include "StdAfx.h"
#include "Interpolator3dx.h"
#include "Serialization\Serialization.h"

void SplineDataBool::serialize(Archive& ar)
{
	MergeBlocksAuto merge(ar);
	ar.serialize(tbegin, "tbegin", "tbegin");
	ar.serialize(inv_tsize, "inv_tsize", "inv_tsize");
	ar.serialize(value, "value", "value");
}

