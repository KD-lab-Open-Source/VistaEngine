#ifndef __UNIQUE_CONTAINER_H__
#define __UNIQUE_CONTAINER_H__

#include <vector>
#include <algorithm>

using namespace std;

template<class T>
class UniqueContainer
{
public:
	typedef std::vector<T> Container;
	typedef typename Container::iterator iterator;

	iterator find(T element)
	{
		return std::find(container_.begin(), container_.end(), element);
	}

	bool add(T element)
	{
		if(find(element) == container_.end()){
			container_.push_back(element);
			return true;
		}

		return false;
	}

	bool erase(T element)
	{
		Container::iterator it; 
		if((it = find(element)) != container_.end()){
			container_.erase(it);
			return true;
		}

		return false;
	}

	void clear() { container_.clear(); }

	Container& container() { return container_;}

private:
	Container container_;
};

#endif //__UNIQUE_CONTAINER_H__
