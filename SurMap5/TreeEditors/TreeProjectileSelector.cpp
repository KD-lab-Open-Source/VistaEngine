#include "stdafx.h"
#include "TreeEditor.h"
#include "..\AttribEditor\AttribEditorDlg.h"

#include "EditArchive.h"

#include "TypeLibrary.h"
#include "..\Units\UnitAttribute.h"
#include "TreeSelector.h"

#include "..\Units\IronBullet.h"

#include "..\BookmarkProjectile.h"

typedef AttributeProjectileReference ReferenceType;
typedef AttributeProjectileTable StringTableType;

class ProjectileTreeBuilder : public TreeBuilder {
public:
	ProjectileTreeBuilder(AttributeProjectileReference& reference)
	: reference_(reference)
	, result_(0)
	{}
protected:
	AttributeProjectileReference& reference_;
	ItemType result_;

    ItemType buildTree(TreeCtrl& tree){
		ItemType selectedItem = 0;
		{
			AttributeProjectileTable::Strings::iterator it;
			AttributeProjectileTable::Strings& strings = const_cast<AttributeProjectileTable::Strings&>(AttributeProjectileTable::instance().strings());
			
			FOR_EACH(strings, it) {
				AttributeProjectile& val = *it;

				ItemType item = tree.addTreeObjectFiltered(val, val.c_str(), TLI_ROOT);
				if(&val == &*reference_)
					selectedItem = item;
			}
		}
		return selectedItem;
    }
	bool select(TreeCtrl& tree, ItemType item){
		if(AttributeProjectile* obj = tree.objectByItem(item)->get<AttributeProjectile*>()){
			reference_ = AttributeProjectileReference(obj->c_str());
			return true;
		}
		else{
			return false;
		}
	}
private:
};

class TreeProjectileSelector : public TreeEditorImpl<TreeProjectileSelector, ReferenceType> {
public:
	bool invokeEditor(ReferenceType& reference, HWND parent)
	{
		CTreeSelectorDlg dlg(CWnd::FromHandle(parent));
		ProjectileTreeBuilder treeBuilder(reference);
		dlg.setBuilder(&treeBuilder);
		dlg.DoModal();
		return true;
	}
	bool hideContent () const{
		return true;
	}
	Icon buttonIcon() const{
		return TreeEditor::ICON_REFERENCE;
	}
	virtual std::string nodeValue () const { 
		return getData().c_str();
	}
	bool hasLibrary() const{
		return true;
	}
	bool visitLibrary(){
		AttribEditorInterface& attribEditorInterface();

		ShareHandle<BookmarkBase> bookmark = new BookmarkProjectile(getData());
		attribEditorInterface().visitBookmark(bookmark);
		bookmark = 0;
		return true;
	}
};


REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(AttributeProjectileReference).name (), TreeProjectileSelector);
