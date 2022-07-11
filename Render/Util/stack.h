#pragma once

#include "Render\inc\rd.h"

#ifdef _DEBUG

class RENDER_API CreateStack
{
public:
	enum {max_size=16};
protected:
	DWORD pointers[max_size];
public:
	CreateStack();
	void GetStack(std::string& s);
};
#else 
class RENDER_API CreateStack
{
public:
	void GetStack(std::string& s){s.clear();};
};

#endif