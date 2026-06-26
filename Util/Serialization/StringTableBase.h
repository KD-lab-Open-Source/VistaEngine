#pragma once

#include "XTL/Handle.h"
#include "Serialization/Serializer.h"

//////////////////////////////////////////////////////////////
// String ������ ����� �����������(const char* = ""), c_str() � 
// ������������. � ����� ������, ��� ����� ���� �� ������ ������,
// �� � �������������� ������.
//
// Hints: 1) ����� ������������ ���������� ���������� ������������
// ������, �.�. �������� ��������� �������� ����.
//////////////////////////////////////////////////////////////
class StringTableBase
{
public:
	StringTableBase(const char* name) : name_(name), index_(-1) {}
	const char* c_str() const { return name_.c_str(); }

	void serialize(Archive& ar);

	void setName(const char* name) { name_ = name; }

	void setStringIndex(int index) { index_ = index; }
	int stringIndex() const { return index_; }

	// ��� ���������:
	const char*			editorName() const{ return c_str(); }
	void				editorCreate(const char* name, const char* groupName) { editorSetGroup(groupName); }
	template<class T>
	Serializer       editorSerializer(T* self, const char* name, const char* nameAlt, bool protectedName);
	string				editorGroupName() const{ return ""; }
	static void			editorGroupMoveBefore(int index, int beforeIndex) {}
	static const char*  editorGroupsComboList() { return 0; }
	static void			editorAddGroup(const char*) { xassert(0); }
	static bool		    editorAllowDrag() { return true; }
	static bool			editorAllowRename() { return true; }
	virtual void        editorSetGroup(const char* group) {}
	virtual bool		editorVisible() const{ return true; }
	static bool		    editorDynamicGroups() { return false; }

protected:
	string name_;
	int index_;

	template<class String> friend class StringTable;
};

// ��� ������������ ��������� ��� �������������� ������
class StringTableBaseSimple : public StringTableBase
{
public:
	StringTableBaseSimple(const char* name = "") : StringTableBase(name) {}

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	// ��� ���������:
	template<class T>
	Serializer editorSerializer(T* self, const char* name, const char* nameAlt, bool protectedName) { return Serializer(name_, name, nameAlt); }
};

template<class T>
class StringTableBasePolymorphic : public StringTableBase
{
public:
	StringTableBasePolymorphic(const char* name = "", T* type = 0) : StringTableBase(name), type_(type) {}

	void serialize(Archive& ar);

	T* get() const { return type_; }
	void set(T* type) { type_ = type; }
	
	// ��� ���������:
	template<class U>
	Serializer       editorSerializer(U* self, const char* name, const char* nameAlt, bool protectedName);
	template<class U>
	static void			editorGroupMoveBefore(int index, int beforeIndex) { xassert(0); }
	string         editorGroupName() const{ return type_ ? FactorySelector<T>::Factory::instance().nameAlt(typeid(*type_).name()) : ""; }
	void                editorSetGroup(const char* group);
	static const char*  editorGroupsComboList() { return FactorySelector<T>::Factory::instance().comboListAlt(); }
	static bool		    editorAllowDrag() { return false; }

protected:
	PolymorphicHandle<T> type_;
};
