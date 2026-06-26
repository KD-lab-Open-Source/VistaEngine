#ifndef __UNIQUE_VECTOR_H__
#define __UNIQUE_VECTOR_H__

#include <vector>
#include <algorithm>

template<class T>
class UniqueVector : public std::vector<T>
{
	typedef vector<T> BaseClass;

public:
	typename std::vector<T>::iterator find(const T& element)
	{
		return std::find(this->begin(), this->end(), element);
	}

	void add(const T& element)
	{
		if(this->find(element) == this->end())
			this->push_back(element);
	}

	void remove(const T& element)
	{
		typename std::vector<T>::iterator it = this->find(element);
		if(it != this->end())
			BaseClass::erase(it);
	}

	bool exists(const T& element)
	{
		typename std::vector<T>::iterator it = this->find(element);
		return it != this->end();
	}

	bool serialize(Archive& ar, const char* name, const char* nameAlt)
	{
		return ar.serialize((BaseClass&)*this, name, nameAlt);
	}
};

#endif //__UNIQUE_VECTOR_H__
