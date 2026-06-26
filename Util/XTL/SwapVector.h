#ifndef __SWAP_VECTOR_H__
#define __SWAP_VECTOR_H__

// ������ � ���������� erase 
// ��� ��������������� �������������������
template<class T>
class SwapVector : public vector<T>
{
public:
	typedef typename vector<T>::iterator iterator;

	iterator erase(iterator it) {
		iterator last = this->end();
		--last;
        if(it != last){
			std::swap(*it, *last);
			int index = it - this->begin();
			this->pop_back();
			return this->begin() + index;
		}
		else{
			this->pop_back();
			return this->end();
		}
	}

	void erase(const T& t) {
		iterator i = std::find(this->begin(), this->end(), t);
		if(i != this->end())
			erase(i);
		xassert(std::find(this->begin(), this->end(), t) == this->end());
	}

	bool serialize(Archive& ar, const char* name, const char* nameAlt) {
		return ar.serialize(static_cast<vector<T>&>(*this), name, nameAlt);
	}
};

#endif // __SWAP_VECTOR_H__

