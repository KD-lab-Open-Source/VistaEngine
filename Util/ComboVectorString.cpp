#include "stdafx.h"
#include "ComboVectorString.h"
#include "Serialization.h"

string getStringTokenByIndex(const char* worker, int number);
int indexInComboListString(const char* combo_string, const char* value);

ComboVectorString::ComboVectorString(vector<string> strings, int value, bool zeroValue)
: ComboListString()
{
	string comboList = zeroValue ? "|" : "";
	vector<string>::iterator it;
	FOR_EACH(strings, it){
		if(it != strings.begin())
			comboList += "|";
		comboList += it->c_str();
	}
	setComboList(comboList.c_str());
	(ComboListString&)(*this) = (value >= strings.size() ? "" : strings[value]);
}

ComboVectorString::ComboVectorString(const char* lst, int value)
: ComboListString(lst)
{
	(ComboListString&)(*this) = getStringTokenByIndex(lst, value);
}

int ComboVectorString::value() const
{
	return indexInComboListString(comboList(), (const char*)(*this));
}

bool ComboVectorString::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize((ComboListString&)(*this), name, nameAlt);
}
