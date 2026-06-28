#include "xglobal.h"
#include <cstdint>

XStream& XStream::operator< (const char* v)
{
	write(v,(unsigned)strlen(v));
	return *this;
}

XStream& XStream::operator< (char v)
{
	write(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator< (unsigned char v)
{
	write(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator< (short v)
{
	write(&v,(unsigned)sizeof(short));
	return *this;
}

XStream& XStream::operator< (unsigned short v)
{
	write(&v,(unsigned)sizeof(unsigned short));
	return *this;
}

XStream& XStream::operator< (int v)
{
	write(&v,(unsigned)sizeof(int));
	return *this;
}

XStream& XStream::operator< (unsigned v)
{
	write(&v,(unsigned)sizeof(unsigned));
	return *this;
}

XStream& XStream::operator< (long v)
{
	// Wire width of long is fixed at 4 bytes (32-bit Windows layout) so files
	// round-trip across 32/64-bit. On 32-bit this is identical to sizeof(long).
	int32_t v32 = (int32_t)v;
	write(&v32,(unsigned)sizeof(v32));
	return *this;
}

XStream& XStream::operator< (unsigned long v)
{
	uint32_t v32 = (uint32_t)v;
	write(&v32,(unsigned)sizeof(v32));
	return *this;
}

XStream& XStream::operator< (double v)
{
	write(&v,(unsigned)sizeof(double));
	return *this;
}

XStream& XStream::operator< (float v)
{
	write(&v,(unsigned)sizeof(float));
	return *this;
}

XStream& XStream::operator< (long double v)
{
	// long double is 8 bytes on 32-bit MSVC but 16 on clang; pin the wire to 8.
	double v64 = (double)v;
	write(&v64,(unsigned)sizeof(v64));
	return *this;
}




