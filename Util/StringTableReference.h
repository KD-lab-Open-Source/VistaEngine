#ifndef __STRING_TABLE_REFERENCE_H__
#define __STRING_TABLE_REFERENCE_H__

template<class String>
class StringTable;

template<class String, bool canAlwaysDereference>
class StringTableReferenceBase
{
public:
	typedef String StringType;
	typedef StringTable<String> StringTableType;

	StringTableReferenceBase() { key_ = canAlwaysDereference? 0 : -1; }
	explicit StringTableReferenceBase(const char* name);

	int key() const { return key_; }
	void setKey(int key) { key_ = key; }

	const char* c_str() const;

	bool operator==(const StringTableReferenceBase& rhs) const {
		return key_ == rhs.key_;
	}

	bool operator<(const StringTableReferenceBase& rhs) const {
		return key_ < rhs.key_;
	}

    const char* comboList() const;
	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	virtual bool refineComboList() const { return false; } 
	virtual bool validForComboList(const String& data) const { return true; }
	virtual const char* editorTypeName() const { return ""; }
	static bool canAlwaysBeDerefenced() { return canAlwaysDereference; }

protected:
	const String* getInternal() const;
	void init(const char* name);

private:
	int key_;

	friend StringTable<String>;
};

template<class String, bool canAlwaysDereference>
class StringTableReference : public StringTableReferenceBase<String, canAlwaysDereference>
{
	typedef StringTableReferenceBase<String, canAlwaysDereference> BaseClass;
public:
	StringTableReference() {}
	explicit StringTableReference(const char* name) : BaseClass(name) {}

	const char* editorTypeName() const { return typeid(StringTableReference).name(); }

	const String* get() const { return getInternal(); }
	const String* operator->() const { return get(); }
	const String& operator*() const { return *get(); }
	operator const String*() const { return get(); }
};

#endif //__STRING_TABLE_REFERENCE_H__
