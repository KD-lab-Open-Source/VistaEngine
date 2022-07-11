#ifndef __OBJECTS_TREE_H_INCLUDED__
#define __OBJECTS_TREE_H_INCLUDED__

#include <typeinfo>
#include "TreeListCtrl.h"
#include "Handle.h"

#ifndef __DISABLE_SERIALIZEABLE__
# include "SerializationTypes.h"
# include "Serializeable.h"
#endif

template<class T>
struct RemovePointer{
	typedef T type;
};

template<class T>
struct RemovePointer<T*>{
	typedef T type;
};


class TreeObject;
typedef std::vector<TreeObject*> TreeObjects;

class CObjectsTreeCtrl : public CTreeListCtrl {
	friend TreeObject;
	DECLARE_DYNAMIC(CObjectsTreeCtrl)
public:
	typedef CTreeListItem* ItemType;
    static const char* className() { return "VistaEngineObjectsTreeCtrl"; }
	
	CObjectsTreeCtrl ();

	virtual void onRightClick() {}

    bool initControl(DWORD dwStyle);
	void insertColumn(const char* name, int width = 0);
    void clear();
	void deleteObject(TreeObject* object);
	TreeObject* rootObject() { return rootObject_; }
	ItemType rootItem() const{ return TLI_ROOT; }
	const TreeObject* rootObject() const{ return rootObject_; }

	void selectObject(TreeObject* object);

	// obsolete interface:
    template<class T>
    ItemType addObject(const T& _object, const char* _name, ItemType _root_handle = TLI_ROOT, ItemType _after = TLI_LAST);
    ItemType addObject(TreeObject* new_object, ItemType _root_handle = TLI_ROOT, ItemType _after = TLI_LAST);
    template<class T>
    TreeObject* findObjectByData(const T& data);

	void expandParents(ItemType item);
	void focusItem(ItemType item);

	template<class T> ItemType findItemByData(T& data);
	template<class T> void focusByData(T& data);
	template<class T> void deleteByData(T& data);

    void deleteChildItems(ItemType item);

	ItemType objectItem(TreeObject* object);
    TreeObject* objectByItem(ItemType item);
    template<class T> T* dataFromItem(ItemType item);
	// ^^^

	TreeObjects selection();
	TreeObject* selected();
	void overrideRoot(TreeObject* newRoot);
protected:

    afx_msg void OnBeginDrag(NMHDR* nm, LRESULT* result);
    afx_msg void OnDragEnter(NMHDR* nm, LRESULT* result);
    afx_msg void OnDragLeave(NMHDR* nm, LRESULT* result);
    afx_msg void OnDragOver(NMHDR* nm, LRESULT* result);
    afx_msg void OnDrop(NMHDR* nm, LRESULT* result);
    afx_msg void OnBeginLabelEdit(NMHDR* nm, LRESULT* result);
    afx_msg void OnItemCheck(NMHDR* nm, LRESULT* result);
    afx_msg void OnItemRClick(NMHDR* nm, LRESULT* result);
	afx_msg void OnSelChanged(NMHDR* nm, LRESULT* result);
	afx_msg void OnItemDblClick(NMHDR* nm, LRESULT* result);
	DECLARE_MESSAGE_MAP();
private:
	typedef std::vector< ShareHandle<TreeObject> > Objects;

    TreeObject* addObject(TreeObject* newObject, TreeObject* parentObject, TreeObject* after = 0);
	void registerWindowClass();

	TreeObjects draggedObjects_;

	ShareHandle<TreeObject> rootObject_;
	Objects objects_;
};


class TreeObject : public ShareHandleBase{
	friend class TreeRootObject;
	friend class CObjectsTreeCtrl;
public:
    TreeObject ()
	: tree_(0)
	, item_(0)
	{}

	virtual void onSelect(){}
	virtual void onRightClick(){}
	virtual bool onDoubleClick(){ return false; }

	virtual void onDrop(const TreeObjects& objects) {}
	virtual bool onBeginDrag() { return false; }
	virtual bool onDragOver(const TreeObjects& objects){ return false; }
	virtual void onEndDrag(const TreeObjects& objects){}

	virtual const std::type_info& getTypeInfo () const { return typeid(TreeObject); }
	virtual void* getPointer() const { return reinterpret_cast<void*>(const_cast<TreeObject*>(this)); }
	#ifndef __DISABLE_SERIALIZEABLE__
	virtual Serializeable getSerializeable(const char* name = "", const char* nameAlt = "") { return Serializeable(); }
	#endif

    virtual const char* name () const {
        return name_.c_str();
    }

	class iterator{/*{{{*/
		friend TreeObject;
	public:
		iterator()
		: item_(0)
		, tree_(0)
		{}
		TreeObject* operator*(){
			return tree_->objectByItem(item_);
        }

		iterator& operator++(){
			item_ = tree_->GetNextItem(item_, TLGN_NEXT);
			return *this;
		}
		iterator operator++(int){
			iterator old(*this);
			++(*this);
			return old;
		}

		bool operator!=(iterator rhs) const{
			return rhs.tree_ != tree_ || rhs.item_ != item_;
		}
	private:
		iterator(CObjectsTreeCtrl* tree, CObjectsTreeCtrl::ItemType item)
		: item_(item)
		, tree_(tree)
		{
		}

        CObjectsTreeCtrl* tree_;
		CObjectsTreeCtrl::ItemType item_;
	};/*}}}*/

	iterator begin(){
		if(tree_ && item_ == TLI_ROOT){
			if(tree_->GetItemCount())
				return iterator(tree_, tree_->GetChildItem(item_));
			else
				return iterator(tree_, 0);		
		}
		else
		return iterator(tree_, tree_->GetChildItem(item_));
	}

	iterator end(){
		return iterator(tree_, 0);
	}

	std::size_t size() const;
	bool empty() const;

	void erase();

	void showCheckBox(bool show);
	void setImage(int defaultImage, int expandedImage, int selectedImage, int expandedSelectedImage);
	void setCheck(bool checked);

	template<class TreeObjectDerived>
	TreeObjectDerived* add(TreeObjectDerived* newChild, TreeObject* after = 0){
		xassert(tree_ && item_);
		tree_->addObject(newChild, this, after);
		return newChild;
	}

	TreeObject* parent();
	const TreeObject* parent() const;
    void clear(){
        xassert(tree_ && item_);
        tree_->deleteChildItems(item_);
    }
	void updateLabel();
	void expandParents();
	virtual void remove() { xassert(0); }
	void focus();
	bool selected() const;

	void ensureVisible(){
        xassert(tree_ && item_);
		tree_->EnsureVisible(item_, 0);
	}

	void expand(){
        xassert(tree_ && item_);
		if(item_ != TLI_ROOT)
			tree_->Expand(item_, TLE_EXPAND);
	}
	
	void expandAll(){
		expand();
		iterator it;
		FOR_EACH(*this, it){
			(*it)->expandAll();
		}
	}


	template<class T>
    T get(){
		typedef ::RemovePointer<T>::type NonPointerType;
		if (typeid(NonPointerType) == getTypeInfo ()) {
            return static_cast<NonPointerType*>(getPointer ());
        }
        return 0;
    }
    virtual ~TreeObject(){}
protected:
	CObjectsTreeCtrl::ItemType item() const{ return item_; };


	mutable std::string name_;
	CObjectsTreeCtrl* tree_;
	CObjectsTreeCtrl::ItemType item_;
private:
	friend CObjectsTreeCtrl;

	void addedToTree(CObjectsTreeCtrl* tree, CObjectsTreeCtrl::ItemType item){
		tree_ = tree;
		item_ = item;
	}

};

typedef TreeObject TreeObjectBase;

template<class Type, class Base = TreeObject>
class TreeObjectImpl : public Base {
public:
	virtual const std::type_info& getTypeInfo () const {
		return typeid(Type);
	}
    TreeObjectImpl (const Type& _data, const char* _name) : data_ (const_cast<Type&> (_data)) {
        name_ = _name;
    }

    void* getPointer()const { return reinterpret_cast<void*>(&data_); }
	#ifndef __DISABLE_SERIALIZEABLE__
	Serializeable getSerializeable(const char* name = "", const char* nameAlt = "") {
		return Serializeable(data_, name, nameAlt);
	}
	#endif
protected:
    Type& data_;
};


template<class T>
T* CObjectsTreeCtrl::dataFromItem(ItemType item)
{
    return objectByItem(item).get<T>();
}


template<class T>
TreeObject* CObjectsTreeCtrl::findObjectByData(const T& data)
{
    void* ptr = static_cast<void*>(&const_cast<T&>(data));
    Objects::iterator it;
    FOR_EACH(objects_, it) {
        if((*it)->getPointer() == ptr)
            return (*it);
    }
    return 0;
}

template<class T>
CObjectsTreeCtrl::ItemType CObjectsTreeCtrl::addObject(const T& _object, const char* _name, ItemType _root_handle, ItemType _after)
{
    return addObject(new TreeObjectImpl<T>(_object, _name), _root_handle, _after);
}

template<class T>
void CObjectsTreeCtrl::focusByData(T& data)
{
	if(ItemType item = findItemByData(data))
		focusItem(item);
}

template<class T>
CObjectsTreeCtrl::ItemType CObjectsTreeCtrl::findItemByData(T& data)
{
	if(TreeObject* object = findObjectByData(data))
		return object->item();
	else{
		xassert(0);
		return 0;
	}
}

template<class T>
void CObjectsTreeCtrl::deleteByData(T& data)
{
	TreeObject* object = findObjectByData(data);
	xassert(object);
	if(object)
		deleteObject(object);
}


#endif
