#ifndef __LIBRARY_WRAPPER_H__
#define __LIBRARY_WRAPPER_H__

#include "Serializeable.h"
#include "LibrariesManager.h"

string getStringTokenByIndex(const char*, int);

class Archive;
class LibraryWrapperBase 
{
	template<class T>
	friend class LibraryWrapper;
	template<class T>
	friend class LocLibraryWrapper;
public:
	LibraryWrapperBase();
	void loadLibrary();
	void saveLibrary();
	bool editLibrary(bool translatedOnly = false);

	unsigned int crc() { return crc_; }

	const char* editName() { return editName_; }
	const char* name() { return sectionName_; }
	bool visibleInEditor() const{ return visibleInEditor_; }
protected:
	virtual void serializeLibrary(Archive& ar) = 0;

	const char* sectionName_;
	string fileName_;
	int version_;
	const char* editName_;
	const char* name_;
	unsigned int crc_;
	bool saveTextOnly_;
	bool visibleInEditor_;
};

class EditorLibraryInterface : public LibraryWrapperBase{
public:
	class EditorIterator{
		friend EditorLibraryInterface;
	public:
		Serializeable operator*(){
			if(index_ >= library_->editorSize()){
				xassert(0);
				return Serializeable();
			}
			else
				return library_->editorElementSerializeable(index_, "", "", true);
		}
		EditorIterator& operator++(int){
            ++index_;
			return *this;
		}
		EditorIterator operator++(){
			EditorIterator temp = *this;
            ++index_;
			return temp;
		}

		std::string name() const{
			return library_->editorElementName(index_);
		}

		bool operator==(const EditorIterator& rhs)const{
			xassert(library_ == rhs.library_);
			return (index_ == rhs.index_);
		}
		bool operator!=(const EditorIterator& rhs)const{
			xassert(library_ == rhs.library_);
			return (index_ != rhs.index_);
		}
	protected:
		EditorIterator(EditorLibraryInterface* library, int index)
		: library_(library)
		, index_(index)
		{
		}
	private:
		int index_;
		EditorLibraryInterface* library_;
	};
	

	EditorIterator editorBegin(){ return EditorIterator(this, 0); }
	EditorIterator editorEnd(){ return EditorIterator(this, editorSize()); }

    // editor methods
	virtual bool            editorDynamicGroups() const { return false; }
	bool					editorAllowDrag() { return !editorDynamicGroups(); }
	virtual bool			editorAllowRename() const{ return true; }
    virtual int             editorFindElement(const char* elementName) const;

	virtual std::string     editorAddElement(const char* name, const char* group = "") { return ""; };
	virtual void			editorAddGroup(const char* name) { xassert(0); };
	virtual void            editorElementErase(int index) {}
	void					editorElementErase(const char* name);
	virtual void            editorElementMoveBefore(int index, int beforeIndex) {};
	void					editorElementMoveBefore(const char* name, const char* beforeName);
    
	virtual const char*     editorGroupsComboList() const { return 0; }
	virtual std::string     editorGroupName(int index) const{ return ""; }
	virtual int				editorGroupIndex(const char* name) { return -1; }
	void					editorGroupMoveBefore(int index, int beforeIndex) {}
	virtual const char*     editorComboList() const { return ""; }
	virtual std::size_t     editorSize() const { return 0; }

	virtual const char*     editorElementName(int index) const{ xassert(0); return ""; }
	virtual void            editorElementSetName(int index, const char* newName) { xassert(0); }
	void                    editorElementSetName(const char* oldName, const char* newName);

	virtual Serializeable   editorElementSerializeable(int index, const char* name, const char* nameAlt, bool protectedName){ return Serializeable(); };
    Serializeable           editorElementSerializeable(const char* elementName, const char* name, const char* nameAlt, bool protectedName);

	virtual std::string     editorElementGroup(int index) const{ return ""; }
	std::string             editorElementGroup(const char* name) const;

	virtual void            editorElementSetGroup(int index, const char* group) {};
	void                    editorElementSetGroup(const char* name, const char* group);
	virtual void			editorSort() {}

};

template<class T>
class LibraryWrapper : public EditorLibraryInterface
{
public:
	void serializeLibrary(Archive& ar);

	static T& instance();
};

#define WRAP_LIBRARY(Type, sectionName, editName, fileName, version, editable)	\
	template<> Type& LibraryWrapper<Type>::instance() {							\
		static Type* t;															\
		if(!t){																	\
			static Type tt;														\
			t = &tt;															\
			t->sectionName_ = sectionName;										\
			t->visibleInEditor_ = editable;										\
			t->editName_ = editName;											\
			t->fileName_ = fileName;											\
			t->version_ = version;												\
			t->loadLibrary();													\
		}																		\
		return *t;																\
	}																			\
	template<> void LibraryWrapper<Type>::serializeLibrary(Archive& ar) {		\
		ar.serialize(instance(), sectionName_, "Библиотека");					\
	}																			\
    namespace{                                                                  \
		bool registered_##Type = LibrariesManager::instance().registerLibrary(sectionName, (LibrariesManager::LibraryInstanceFunc)(&LibraryWrapper<Type>::instance), editable);       \
    };



class LocLibraryWrapperBase : public LibraryWrapperBase
{
	template<class T>
		friend class LocLibraryWrapper;
public:
	LocLibraryWrapperBase();
	void loadLanguage(const char* language);

	static string& locDataRootPath();

protected:
	string language_;
	const char* file_;
};


template<class T>
class LocLibraryWrapper : public LocLibraryWrapperBase
{
public:
	void serializeLibrary(Archive& ar);

	static T& instance();
};


#define WRAP_LOC_LIBRARY(Type, sectionName, editName, language, fileName, version)	\
	template<> Type& LocLibraryWrapper<Type>::instance() {							\
		static Type* t;																\
		if(!t){																		\
			static Type tt;															\
			t = &tt;																\
			t->sectionName_ = sectionName;											\
			t->editName_ = editName;												\
			t->file_ = fileName;													\
			t->version_ = version;													\
			t->loadLanguage(language);												\
		}																			\
		return *t;																	\
	}																				\
	template<> void LocLibraryWrapper<Type>::serializeLibrary(Archive& ar) {		\
	ar.serialize(instance(), sectionName_, "Библиотека");							\
	}																				\

#endif //__LIBRARY_WRAPPER_H__
