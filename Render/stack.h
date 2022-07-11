#pragma once

#ifdef _DEBUG

struct CreateStack
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
struct CreateStack
{
	void GetStack(std::string& s){s.clear();};
};

#endif