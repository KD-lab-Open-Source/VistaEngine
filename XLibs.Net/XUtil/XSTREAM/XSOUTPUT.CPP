#include "xglobal.h"
#include <cstdint>

XStream& XStream::operator> (char& v)
{
	read(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator> (unsigned char& v)
{
	read(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator> (short& v)
{
	read(&v,(unsigned)sizeof(short));
	return *this;
}

XStream& XStream::operator> (unsigned short& v)
{
	read(&v,(unsigned)sizeof(unsigned short));
	return *this;
}

XStream& XStream::operator> (int& v)
{
	read(&v,(unsigned)sizeof(int));
	return *this;
}

XStream& XStream::operator> (unsigned& v)
{
	read(&v,(unsigned)sizeof(unsigned));
	return *this;
}

XStream& XStream::operator> (long& v)
{
	// long is stored as 4 bytes on the wire (32-bit Windows layout); read and
	// sign-extend. On 32-bit this is identical to reading sizeof(long).
	int32_t v32 = 0;
	read(&v32,(unsigned)sizeof(v32));
	v = v32;
	return *this;
}

XStream& XStream::operator> (unsigned long& v)
{
	uint32_t v32 = 0;
	read(&v32,(unsigned)sizeof(v32));
	v = v32;
	return *this;
}

XStream& XStream::operator> (double& v)
{
	read(&v,(unsigned)sizeof(double));
	return *this;
}

XStream& XStream::operator> (float& v)
{
	read(&v,(unsigned)sizeof(float));
	return *this;
}

XStream& XStream::operator> (long double& v)
{
	// long double is 8 bytes on the wire (32-bit MSVC layout), 16 on clang.
	double v64 = 0;
	read(&v64,(unsigned)sizeof(v64));
	v = v64;
	return *this;
}




