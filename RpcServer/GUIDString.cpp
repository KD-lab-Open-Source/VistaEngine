#include "stdafx.h"
#include "GUIDString.h"

GUIDString::GUIDString(const GUID& g)
{
	static_cast<GUID&>(*this) = g;

	putByte(Data1 >> 3 * 8);
	putByte(Data1 >> 2 * 8);
	putByte(Data1 >> 1 * 8);
	putByte(Data1 >> 0 * 8);
	guidStr_.push_back('-');
	putByte(Data2 >> 1 * 8);
	putByte(Data2);
	guidStr_.push_back('-');
	putByte(Data3 >> 1 * 8);
	putByte(Data3);
	guidStr_.push_back('-');
	putByte(Data4[0]);
	putByte(Data4[1]);
	guidStr_.push_back('-');
	putByte(Data4[2]);
	putByte(Data4[3]);
	putByte(Data4[4]);
	putByte(Data4[5]);
	putByte(Data4[6]);
	putByte(Data4[7]);
}

GUIDString::GUIDString(const char* s)
{
	guidStr_ = s;

	if(guidStr_.size() != 36)
		return;

	static int shift[] = {3, -1, -1, -1, 0, 5, -1, 0, 3, -1, 0, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1};

	BYTE* pg = reinterpret_cast<BYTE*>(this);
	const char* str = guidStr_.c_str();
	for(int i = 0; i < 20; ++i){
		pg += shift[i];
		if(*str != '-'){
			*pg = fromHexChar(*str);
			*pg <<= 4;
			++str;
			*pg |= fromHexChar(*str);
		}
		++str;
	}
}