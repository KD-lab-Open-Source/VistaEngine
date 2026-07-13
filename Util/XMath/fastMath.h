#pragma once

//////////////////////////////////////////////////////////////////////////////  
// FAST INVERSE SQUARE ROOT
// By Chris Lomont
// http://www.lomont.org/Math/Papers/2003/InvSqrt.pdf
//
// ������������ ����������� 0.0017758, �������������� � 6 ��� ������� 1/sqrtf()
//////////////////////////////////////////////////////////////////////////////  
inline float invSqrtFast(float x)
{
	x += 1e-7f; // �������, ����������� ������� �� 0
	float xhalf = 0.5f*x;
	int i = *(int*)&x; // get bits for floating value
	i = 0x5f375a86 - (i>>1); // gives initial guess y0
	x = *(float*)&i; // convert bits back to float
	x = x*(1.5f-xhalf*x*x); // Newton step, repeating increases accuracy
	return x;
}

// Was an x87 `fprem` loop on Windows. No 64-bit compiler takes inline asm, and
// fmodf is what the hardware instruction does anyway.
inline float fmodFast(float a, float b)
{
	return fmodf(a, b);
}

inline unsigned int F2DW( float f ) 
{ 
	return *((unsigned int*)&f); 
}

inline float DW2F( unsigned int f ) 
{ 
	return *((float*)&f); 
}

