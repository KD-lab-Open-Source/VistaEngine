#include "stdafx.h"
#include "..\Util\Serialization\AttribEditorInterface.h"
bool net_log_mode;
XBuffer net_log_buffer;
RandomGenerator logicRnd;
XBuffer& watch_buffer()
{
	static XBuffer buffer(1000, 1);
	return buffer;
}

void DrawOnRegion(int layer, const Vect2i& point, float radius)
{
	xassert(0);
}
AttribEditorInterface& attribEditorInterface()
{
    xassert(0);
    return *(AttribEditorInterface*)(0);
}

