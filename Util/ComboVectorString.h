#ifndef __COMBOLISTVECTOR_H_INCLUDED__
#define __COMBOLISTVECTOR_H_INCLUDED__

#include "SerializationTypes.h"
#include <vector>

// выбор строки из вектора строк с возвратом индекса
struct ComboVectorString : public ComboListString
{
	ComboVectorString(vector<string> strings, int value = 0, bool zeroValue = false);
	
	ComboVectorString(const char* lst, int value = 0);

	int value() const;

	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

#endif
