#pragma once
#ifndef __VISTARPC_GUIDSTRING_H_INCLUDED__
#define __VISTARPC_GUIDSTRING_H_INCLUDED__

class GUIDString : protected GUID
{
public:
	typedef unsigned char BYTE;

	GUIDString(const GUID& guid);
	GUIDString(const char* str);

	const GUID& getGUID() const { return static_cast<const GUID&>(*this); }
	const string& getString() const { return guidStr_; }

private:
	BYTE fromHexChar(char a)
	{
		if(a >= '0' && a <= '9')
			return a-'0';
		if(a >= 'A' && a <= 'F')
			return a-'A'+10;
		if(a >= 'a' && a <= 'f')
			return a - 'a' + 10;
		return 0;
	}

	char toHexChar(BYTE b)
	{
		if(b > 9)
			return 'A' + b - 10;
		return '0' + b;
	}

	void putByte(unsigned b)
	{
		guidStr_.push_back(toHexChar((b >> 4) & 0x0F));
		guidStr_.push_back(toHexChar(b & 0x0F));
	}

	string guidStr_;
};

#endif //__VISTARPC_GUIDSTRING_H_INCLUDED__