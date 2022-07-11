//////////////////////////////////////////////
//		Ѕиблиотека типов 
//////////////////////////////////////////////
#ifndef __TYPE_LIBRARY_H__
#define __TYPE_LIBRARY_H__

#include "Handle.h"
#include "StaticMap.h"
#include "Serialization.h"
#include "LibraryWrapper.h"
#include "StringTableReference.h"

#include "Serializeable.h" // дл€ редактора

class Archive;

//////////////////////////////////////////////////////////////
// String должен иметь конструктор(const char* = ""), c_str() и 
// сериализацию. ¬ общем случае, это могут быть не только строки,
// но и дополнительные данные.
//
// Hints: 1) ѕосле сериализации библиотеки необходимо перезапимать
// ссылки, т.к. возможно изменение ключевых имен.
//////////////////////////////////////////////////////////////
class StringTableBase
{
public:
	StringTableBase(const char* name) : name_(name), index_(-1) {}
	const char* c_str() const { return name_.c_str(); }

	void serialize(Archive& ar){
		ar.serialize(name_, "name", "&»м€");
		if(ar.isEdit())
			ar.serialize(index_, "index", 0);
	}

	virtual void setName(const char* name) { name_ = name; }

	void setStringIndex(int index) { index_ = index; }
	int stringIndex() const { return index_; }

	// дл€ редактора:
	const char*			editorName() const{ return c_str(); }
	void				editorCreate(const char* name, const char* groupName) { editorSetGroup(groupName); }
	template<class T>
	Serializeable       editorSerializeable(T* self, const char* name, const char* nameAlt, bool protectedName);
	std::string         editorGroupName() const{ return ""; }
	static void			editorGroupMoveBefore(int index, int beforeIndex) {}
	static const char*  editorGroupsComboList() { return 0; }
	static void			editorAddGroup(const char*) { xassert(0); }
	static bool			editorAllowRename() { return true; }
	virtual void        editorSetGroup(const char* group) {}
	static bool		    editorDynamicGroups() { return false; }
protected:
	string name_;
	int index_;

	template<class String> friend class StringTable;
	template<class String, bool canAlwaysDereference> friend class StringTableReference;
	template<class String, bool canAlwaysDereference> friend class StringTablePolymorphicReference;
};

// ƒл€ наследовани€ объектами без дополнительных данных
class StringTableBaseSimple : public StringTableBase
{
public:
	StringTableBaseSimple(const char* name = "") : StringTableBase(name) {}

	bool serialize(Archive& ar, const char* name, const char* nameAlt){
		if(ar.isEdit()){
			bool nodeExists = ar.openStruct(name, nameAlt, typeid(StringTableBase).name());
			ar.serialize(name_, "name", "&»м€");
			ar.serialize(index_, "index", 0);
			ar.closeStruct(name);
			return nodeExists;
		}
		else
			return ar.serialize(name_, name, nameAlt);
	}

	// дл€ редактора:
	template<class T>
	Serializeable editorSerializeable(T* self, const char* name, const char* nameAlt, bool protectedName) { return Serializeable(name_, name, nameAlt); }
};

template<class String, bool canAlwaysDereference> class StringTableReference;

template<class String>
class StringTable : public LibraryWrapper<StringTable<String> >
{
public:
	typedef String StringType;
	typedef vector<String> Strings;
	typedef Strings Map;

	StringTable();

	void add(const String& data);
	void add(const char* name) { add(String(name)); }
	void remove(const char* name);
	bool exists(const char* name) const { return find(name) != -1; }

	void serialize(Archive& ar);

	const String& operator[](int index) const { return strings_[index];	}

	bool empty() const { return strings_.empty(); }

	int size() const { return strings_.size(); }

	const Strings& strings() const { return strings_; }
	const Strings& map() const { return strings_; }

	void buildComboList();

	const char* comboList() const {	return comboList_.c_str(); }

    // дл€ редактора, virtuals:
    const char*         editorComboList() const{ return comboList_.c_str(); }
	const char*         editorGroupsComboList() const{ return String::editorGroupsComboList(); }
	std::string         editorGroupName(int index) const;
	void				editorGroupMoveBefore(int index, int beforeIndex);
	bool				editorDynamicGroups() const { return String::editorDynamicGroups(); }		
	bool				editorAllowRename() const { return String::editorAllowRename(); }
    std::size_t         editorSize() const { return size(); }

	std::string			editorAddElement(const char* name, const char* group = "");
	void				editorAddGroup(const char* name);
	void                editorElementRemove(int index);
	void                editorElementMoveBefore(int index, int beforeIndex);
	void                editorElementSetName(int index, const char* newName);
	const char*         editorElementName(int index) const;
	void				editorElementErase(int index);
	void				editorSort();

	Serializeable       editorElementSerializeable(int index, const char* name, const char* nameAlt, bool protectedName);
	std::string         editorElementGroup(int index) const;
	void                editorElementSetGroup(int index, const char* group);
private:
	Strings strings_;
	string comboList_;
	string comboListAlwaysDereference_;
	vector<string> headerStrings_;

	const String* find(int key) const;
	int find(const char* name) const;
	const char* findCStr(int key) const;

	friend StringTableReferenceBase<String, true>;
	friend StringTableReferenceBase<String, false>;
};


template<class T>
class StringTableBasePolymorphic : public StringTableBase
{
public:
	typedef T ReferencedType;
	StringTableBasePolymorphic(const char* name = "", T* type = 0) : StringTableBase(name), type_(type) {}

	void serialize(Archive& ar);

	T* get() const { return type_; }
	void set(T* type) { type_ = type; }
	
	// дл€ редактора:
	template<class U>
	Serializeable       editorSerializeable(U* self, const char* name, const char* nameAlt, bool protectedName);
	template<class U>
	static void			editorGroupMoveBefore(int index, int beforeIndex) { xassert(0); }
	std::string         editorGroupName() const{ return type_ ? ClassCreatorFactory<T>::instance().nameAlt(typeid(*type_).name()) : ""; }
	void                editorSetGroup(const char* group);
	static const char*  editorGroupsComboList() { return ClassCreatorFactory<T>::instance().comboListAlt(); }
protected:
	PolymorphicHandle<T> type_;
};

template<class String, bool canAlwaysDereference>
class StringTablePolymorphicReference :	public StringTableReferenceBase<StringTableBasePolymorphic<String>, canAlwaysDereference>
{
	typedef StringTableReferenceBase<StringTableBasePolymorphic<String>, canAlwaysDereference> BaseClass;

public:
	StringTablePolymorphicReference() {}
	explicit StringTablePolymorphicReference(const char* name) : BaseClass(name) {}
	explicit StringTablePolymorphicReference(const String* type);
	
	StringTablePolymorphicReference& operator=(const String* type) { *this = StringTablePolymorphicReference(type); return *this; }

	const String* get() const { 
		const StringTableBasePolymorphic<String>* data = getInternal();
		return data ? data->get() : 0;
	}
	const String* operator->() const { return get(); }
	const String& operator*() const { return *get(); }
	operator const String*() const { return get(); }

	const char* editorTypeName() const { return typeid(StringTablePolymorphicReference).name(); }
	virtual bool validForComboList(const String& data) const { return true; }
	bool validForComboList(const StringTableBasePolymorphic<String>& data) const { return validForComboList(*data.get()); }
};

#endif //__TYPE_LIBRARY_H__
