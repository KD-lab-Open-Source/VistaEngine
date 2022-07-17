#ifndef __TYPE_LIBRARY_IMPL_H__
#define __TYPE_LIBRARY_IMPL_H__

#include "TypeLibrary.h"
#include "TreeEditor.h"
#include "AttribEditorInterface.h"
#include "Console.h"

// _VISTA_ENGINE_EXTERNAL_ - нужно для перевода external-редактора

AttribEditorInterface& attribEditorInterface();

template<class String>
StringTable<String>::StringTable() 
{
	add("");
}

template<class String>
int StringTable<String>::find(const char* name) const
{
	Strings::const_iterator i;		
	FOR_EACH(strings_, i)
		if(!strcmp(i->c_str(), name) && i->index_ != -1) // index устанавливается только после полного прочтения
			return i->index_;

	vector<string>::const_iterator si;
	FOR_EACH(headerStrings_, si)
		if(!strcmp(si->c_str(), name))
			return si - headerStrings_.begin();
	
	return -1;
}

template<class String>
void StringTable<String>::add(const String& data) 
{
	const char* name = data.c_str();
	int maxIndex = 0;
	Strings::iterator i;		
	FOR_EACH(strings_, i){
		if(!strcmp(i->c_str(), name))
			return; 
		maxIndex = max(maxIndex, i->index_);
	}

	//xassert(strings_.empty() || strings_.back().index == maxIndex);
	int new_index = !strings_.empty() ? maxIndex + 1 : 0;;
	strings_.push_back(data);
	strings_.back().index_ = new_index;

	if(strings_.size() > 1){
		comboListAlwaysDereference_ += "|";
		comboList_ += "|";
	}
	comboListAlwaysDereference_ += name;
	comboList_ += name;
}

template<class String>
void StringTable<String>::remove(const char* name) 
{
	Strings::iterator i;		
	FOR_EACH(strings_, i)
		if(!strcmp(i->c_str(), name)){
			strings_.erase(i);
			break;
		}
	if(empty())
		add("");
}

template<class String>
void StringTable<String>::buildComboList()
{
    comboListAlwaysDereference_ = "";
    Strings::iterator i;		
    FOR_EACH(strings_, i){
        if(i != strings_.begin())
            comboListAlwaysDereference_ += "|";
        comboListAlwaysDereference_ += i->c_str();
    }
    comboList_ = "|";
    comboList_ += comboListAlwaysDereference_;
}

template<class String>
void StringTable<String>::serialize(Archive& ar) 
{
	if(!ar.isEdit()){
		if(ar.isOutput()){
			headerStrings_.clear();
			Strings::iterator i;
			FOR_EACH(strings_, i)
				headerStrings_.push_back(i->c_str());
		}

		ar.serialize(headerStrings_, "header", 0);
	}

	ar.serialize(strings_, "strings", editName_);
	
	if(!ar.isEdit() && ar.isInput()) // CONVERSION 27.04.06
		ar.serialize(strings_, "types", 0);

	headerStrings_.clear();

	if(ar.isInput()){
		buildComboList();
		
		if(strings_.empty())
			add("");

		if(!ar.isEdit()){
			int index = 0;
			Strings::iterator i;		
			FOR_EACH(strings_, i)
				i->index_ = index++;
		} else {
            int last_index = 0;
			Strings::iterator i;		
			FOR_EACH(strings_, i)
				if (i->index_ > last_index)
					last_index = i->index_;
			FOR_EACH(strings_, i) {
				if (i->index_ == -1) {
					i->index_ = ++last_index;
				}
			}
		}

		if(ar.isOutput()){ // Инстанцирование кода, никогда не выполняется
			static StringTableReference<String, true> dummyReferenceTrue("");
			dummyReferenceTrue.serialize(ar, "", "");
			static const String& stringTrue = *dummyReferenceTrue;
			static const char* constCharTrue = dummyReferenceTrue.c_str();

			static StringTableReference<String, false> dummyReferenceFalse("");
			dummyReferenceFalse.serialize(ar, "", "");
			static const String& stringFalse = *dummyReferenceFalse;
			static const char* constCharFalse = dummyReferenceFalse.c_str();

			remove("");
		}
	}
}

template<class String>
const String* StringTable<String>::find(int key) const
{
	if(key < 0)
		return 0;
    
	if(key < strings_.size() && strings_[key].index_ == key)
		return &strings_[key];

	Strings::const_iterator i;		
	FOR_EACH(strings_, i)
		if(i->index_ == key)
			return &*i;

	return 0;
}

template<class String>
const char* StringTable<String>::findCStr(int key) const
{
	if(key < 0)
		return "";
    
	if(key < strings_.size() && strings_[key].index_ == key)
		return strings_[key].c_str();

	Strings::const_iterator i;		
	FOR_EACH(strings_, i)
		if(i->index_ == key)
			return i->c_str();

	if(!headerStrings_.empty())
		return headerStrings_[key].c_str();
	else
		return "";
}

template<class T>
class StringTableBaseSerializeableImpl : public SerializeableImpl<T>{
public:
	StringTableBaseSerializeableImpl(const T& object, const char* name, const char* nameAlt, bool protectName)
	: SerializeableImpl<T>(object, name, nameAlt)
	, protectName_(protectName)
	{
	}

    bool serialize(Archive& ar, const char* name, const char* nameAlt){
		std::string stringName;
		if(protectName_)
			stringName = data_.c_str();
		int stringIndex = data_.stringIndex();
		std::string group;

		if(T::editorDynamicGroups())
			group = data_.editorGroupName();

		bool result = ar.serialize(data_, name, nameAlt);

		if(protectName_)
			data_.setName(stringName.c_str());	
		data_.setStringIndex(stringIndex);
		if(T::editorDynamicGroups())
			data_.editorSetGroup(group.c_str());
		return result;
	}
protected:
	bool protectName_;
};

template<class T>
Serializeable StringTableBase::editorSerializeable(T* self, const char* name, const char* nameAlt, bool protectName)
{
	//return Serializeable(*self, name, nameAlt);
	Serializeable result; 
	result.setImpl(new StringTableBaseSerializeableImpl<T>(*self, name, nameAlt, protectName));
	return result;
}

template<class String, bool canAlwaysDereference>
StringTableReferenceBase<String, canAlwaysDereference>::StringTableReferenceBase(const char* name) 
{
	init(name);
//	xassertStr(!strlen(name) || !strcmp(name, "None") || key_ != -1 && "Не найдена строка в таблице: ", name);
	if(canAlwaysDereference){
		if(key_ == -1)
			key_ = 0;
	}
	else if(!strlen(name))
		key_ = -1;
}

template<class String, bool canAlwaysDereference>
const String* StringTableReferenceBase<String, canAlwaysDereference>::getInternal() const 
{
	const String* data = StringTable<String>::instance().find(key_);
	if(data)
		return data;
	else if(canAlwaysDereference){
		xassertStr(0 && "Не найдена строка в таблице, используется первая", StringTable<String>::instance().editName());
		return &StringTable<String>::instance().strings().front();
	}
	else
		return 0;
}

template<class String, bool canAlwaysDereference>
const char* StringTableReferenceBase<String, canAlwaysDereference>::c_str() const 
{ 
	return StringTable<String>::instance().findCStr(key_);
}

template<class String, bool canAlwaysDereference>
const char* StringTableReferenceBase<String, canAlwaysDereference>::comboList() const
{
	return canAlwaysDereference ? StringTable<String>::instance().comboListAlwaysDereference_.c_str() : StringTable<String>::instance().comboList_.c_str();
}

template<class String, bool canAlwaysDereference>
void StringTableReferenceBase<String, canAlwaysDereference>::init(const char* name)
{
	key_ = StringTable<String>::instance().find(name);
}

template<class String, bool canAlwaysDereference>
bool StringTableReferenceBase<String, canAlwaysDereference>::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	const StringTable<String>& table = StringTable<String>::instance();
	const char* comboList = "";
	if(ar.isEdit()) {
		const char* typeName = editorTypeName();
		bool editorRegistered = (attribEditorInterface().isTreeEditorRegistered(typeName) != 0);	
		if(editorRegistered){
			bool nodeExists = ar.openStruct (name, nameAlt, typeName);
			ar.serialize(key_, "key", "Ключ");
			if(ar.isOutput()){
				std::string name(c_str());
				ar.serialize(name, "name", "Имя");
			}
			else{
				std::string name;
				ar.serialize(name, "name", "Имя");
				if(strcmp(name.c_str(), table.findCStr(key_)) != 0)
					*this = StringTableReferenceBase(name.c_str());
			}
			ar.closeStruct (name);
			return nodeExists;
		}
		else if(!refineComboList())
			comboList = canAlwaysDereference ? table.comboListAlwaysDereference_.c_str() : table.comboList_.c_str();
		else{
			static string comboListBuffer;
			comboListBuffer = canAlwaysDereference ?  "" : "|";
			bool first = true;
			StringTable<String>::Strings::const_iterator i;		
			FOR_EACH(table.strings(), i){
				if(validForComboList(*i)){
					if(!first)
						comboListBuffer += "|";
					first = false;
					comboListBuffer += i->c_str();
				}
			}
			comboList = comboListBuffer.c_str();
		}
	}
	ComboListString comboStr(comboList, c_str());
	bool nodeExists = ar.serialize(comboStr, name, nameAlt);
	if(ar.isInput()){
		*this = StringTableReferenceBase(comboStr);
		//xassertStr(!strlen(comboStr) || key_ != -1 && "Не найдена строка в таблице: ", comboStr);
		if(strlen(comboStr) && key_ == -1)
			kdWarning("&Shura", XBuffer() < "Не найдена строка в таблице: " < (const char*)comboStr);
	}
	return nodeExists;
}

template<class T>
template<class U>
Serializeable StringTableBasePolymorphic<T>::editorSerializeable(U* self, const char* name, const char* nameAlt, bool editOnly) 
{
	return type_ ? Serializeable(*self->type_, name, nameAlt) : Serializeable();
}

template<class T>
void StringTableBasePolymorphic<T>::editorSetGroup(const char* group) 
{
	const char* name = ClassCreatorFactory<T>::instance().nameByNameAlt(group);
	type_ = (strcmp(group, "") == 0) ? 0 : ClassCreatorFactory<T>::instance().find(name).create();
}


template<class T>
void StringTableBasePolymorphic<T>::serialize(Archive& ar) 
{
	if(ar.isEdit())
		StringTableBase::serialize(ar);
	else // CONVERSION 27.04.06
		ar.serialize(name_, "first", 0);
	ar.serialize(type_, "second", "&Значение");

	if(ar.isInput() && ar.isOutput()){ // Инстанцирование кода, никогда не выполняется
		static StringTablePolymorphicReference<T, true> dummyTrue((const T*)0);
		static StringTablePolymorphicReference<T, false> dummyFalse((const T*)0);
	}
}

template<class String, bool canAlwaysDereference>
StringTablePolymorphicReference<String, canAlwaysDereference>::StringTablePolymorphicReference(const String* type)
{
	typedef StringTable<StringTableBasePolymorphic<String> > Table;
	const Table& table = Table::instance();
	Table::Strings::const_iterator i;		
	FOR_EACH(table.strings(), i)
		if(i->get() == type){
			setKey(i->index_);
			return;
		}

	setKey(-1);
}
// ---------------------------------------------------------------------------

template<class String>
Serializeable StringTable<String>::editorElementSerializeable(int index, const char* name, const char* nameAlt, bool protectedName)
{
	xassert(index >= 0 && index < strings_.size());
	String& str = strings_[index];
	return str.editorSerializeable(&str, name, nameAlt, protectedName);
}

template<class String>
std::string StringTable<String>::editorElementGroup(int index) const
{
	xassert(index >= 0 && index < strings_.size());
	const String& str = strings_[index];
	return str.editorGroupName();
}

template<class String>
void StringTable<String>::editorElementSetGroup(int index, const char* group)
{
    xassert(index >= 0 && index < size());

	String& str = strings_[index];
	str.editorSetGroup(group);
}

template<class String>
std::string StringTable<String>::editorGroupName(int index) const
{
    xassert(index >= 0 && index < size());
	const String& str = strings_[index];
	return str.editorGroupName();
}


template<class String>
void StringTable<String>::editorGroupMoveBefore(int index, int beforeIndex)
{
	String::editorGroupMoveBefore(index, beforeIndex);
}

template<class String>
void StringTable<String>::editorElementErase(int index)
{
    xassert(index >= 0 && index < size());
	remove(editorElementName(index));
	buildComboList();
}

template<class String>
const char* StringTable<String>::editorElementName(int index) const
{
    xassert(index >= 0 && index < size());
	return strings_[index].c_str();
}

template<class String>
void StringTable<String>::editorElementSetName(int index, const char* newName)
{
    xassert(index >= 0 && index < size());
    String& str = strings_[index];
    std::string oldName = str.c_str();
    str.setName(newName);

    buildComboList();
}

template<class String>
void StringTable<String>::editorAddGroup(const char* name)
{
	String::editorAddGroup(name);
}

template<class String>
std::string StringTable<String>::editorAddElement(const char* name, const char* group)
{
    int index = size();
    add(name);
	if(index < size()){
		strings_[index].editorCreate(name, group);
		buildComboList();
		return strings_[index].c_str();
	}
	else{
		xassert(0);
		return "";
	}
}

template<class String>
void StringTable<String>::editorElementMoveBefore(int index, int beforeIndex)
{
    String temp;

	xassert(index >= 0 && index < size());
	xassert(beforeIndex >= 0 && beforeIndex < size());

	temp = strings_[index];
	strings_.erase(strings_.begin() + index);
    strings_.insert(strings_.begin() + beforeIndex, temp);

	buildComboList();
}

struct CStrLess{
	bool operator()(const StringTableBase& a, const StringTableBase& b){
        return stricmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(const std::string& a, const std::string& b){
        return stricmp(a.c_str(), b.c_str()) < 0;
	}
};

template<class String>
void StringTable<String>::editorSort()
{
	std::sort(strings_.begin(), strings_.end(), CStrLess());
	buildComboList();
}

#endif //__TYPE_LIBRARY_IMPL_H__
