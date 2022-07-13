#ifndef __TREE_SELECTOR_H_INCLUDED__
#define __TREE_SELECTOR_H_INCLUDED__

#include "SafeCast.h"
#include "Handle.h"

#include "..\Util\MFC\ObjectsTreeCtrl.h"
class CSizeLayoutManager;


class TreeObjectFilteredBase : public TreeObject{
public:
	TreeObjectFilteredBase();
	bool matching() const { return matchFilter_; }
	TreeObjectFilteredBase* addFiltered(TreeObjectFilteredBase* object, bool sort = false);

	template<class T>
	TreeObjectFilteredBase* add(const T& object, const char* name);

	bool onDoubleClick();
	bool matchFilter(const char* filter);
	bool buildFiltered();
protected:
	bool matchFilter_;
	bool removeIfEmpty_;
	typedef std::vector< ShareHandle <TreeObjectFilteredBase> > Children;
	Children potentialChildren_;
};

class TreeBuilder{
public:
	class Object : public TreeObjectFilteredBase{
	public:
		template<class T>
		Object* add(T& object, const char* name){
			return static_cast<Object*>(TreeObjectFilteredBase::add(object, name));
		}

		Object* add(Object* object, bool sort = false){
			return static_cast<Object*>(addFiltered(object, sort));
		}
		Object(const char* label, const char* key = 0, bool removeIfEmpty = false){
			name_ = label;
			removeIfEmpty_ = removeIfEmpty;
			if(key){
				key_ = key;
				selectable_ = true;
			}
			else{
				selectable_ = false;
			}
		}
		const char* key() const{ return key_.c_str(); }
		bool selectable() const{ return selectable_; }
	protected:
		std::string key_;
		bool selectable_;
	};

	virtual Object* buildTree(Object* root) = 0;
	virtual bool select(Object* object) = 0;
private:
};

template<class T>
class TreeObjectFiltered;

template<class T>
class TreeObjectFiltered : public TreeObjectImpl<T, TreeObjectFilteredBase>{
public:
	TreeObjectFiltered(const T& t, const char* name)
	: TreeObjectImpl<T, TreeObjectFilteredBase>(t, name)
	{

	}
};


class CBuildableTreeCtrl : public CObjectsTreeCtrl{
public:
	CBuildableTreeCtrl();
	TreeBuilder::Object* rootObject();
};


class CTreeSelectorDlg : public CDialog
{
	DECLARE_DYNAMIC(CTreeSelectorDlg)
public:
	CTreeSelectorDlg(CWnd* pParent = NULL);
	virtual ~CTreeSelectorDlg();

	void setBuilder(TreeBuilder* builder);

	CObjectsTreeCtrl& tree();

	enum { IDD = IDD_TREE_SELECTOR };

	virtual void OnOK();

	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFilterEditChange();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	PtrHandle<CBuildableTreeCtrl> tree_;
	PtrHandle<CSizeLayoutManager> layout_;

	DECLARE_MESSAGE_MAP()
private:
	TreeBuilder* builder_;
};

template<class T>
TreeObjectFilteredBase* TreeObjectFilteredBase::add(const T& object, const char* name)
{
	return addFiltered(new TreeObjectFiltered<T>(const_cast<T&>(object), name));
}


#endif
