#include "stdafx.h"
#include "GUIDSerialize.h"
#include "Serialization.h"

//typedef struct _GUID
//{
//	unsigned long  Data1;
//	unsigned short Data2;
//	unsigned short Data3;
//	unsigned char  Data4[8];
//} GUID;

bool GUIDSerialization::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	static char buf[80];
	
	if(ar.isOutput()){
		int size = sprintf(buf,
			"{0x%08lX, 0x%04hX, 0x%04hX, {0x%02wX, 0x%02wX, 0x%02wX, 0x%02wX, 0x%02wX, 0x%02wX, 0x%02wX, 0x%02wX}}",
			g.Data1, g.Data2, g.Data3, g.Data4[0],
			g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
		xassert(size < sizeof(buf));
	}
	else
		*buf = 0;
	
	string data(buf);
	bool res = ar.serialize(data, name, nameAlt);

	if(res && ar.isInput()){
		int rb[8];
		sscanf(data.c_str(),
			"{%lx, %hx, %hx, {%hx, %hx, %hx, %hx, %hx, %hx, %hx, %hx}}",
			&g.Data1, &g.Data2, &g.Data3,
			&rb[0], &rb[1], &rb[2], &rb[3], &rb[4], &rb[5], &rb[6], &rb[7]);
		for(int idx = 0; idx < 8; ++idx)
			g.Data4[idx] = (rb[idx] & 0xFF);
	}

	return res;
}


bool XGUID::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	return ar.serialize(GUIDSerialization(*this), name, nameAlt);
}