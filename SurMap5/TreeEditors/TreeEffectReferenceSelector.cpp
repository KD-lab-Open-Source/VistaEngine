#include "EditArchive.h"

#include "..\LibraryBookmark.h"

#include "TreeEditor.h"
#include "TypeLibrary.h"
#include "TreeSelector.h"
#include "TreeReferenceEditor.h"
#include "EffectReference.h"

class EffectReferenceTreeBuilder : public TreeBuilder{
public:
	ReferenceTreeBuilder(ReferenceType& reference)
	: reference_(reference)
	, result_(0)
	{}
protected:
	ReferenceType& reference_;
	ItemType result_;

	typedef typename EffectContainerLibrary LibraryType;
    ItemType buildTree(TreeCtrl& tree){
		typedef StaticMap<std::string, TreeCtrl::ItemType> Groups;
        Groups groups;
        groups[""] = tree.rootItem();

        LibraryType& library = LibraryType::instance();
		
        LibraryType::Strings& strings = const_cast<LibraryType::Strings&>(library.strings());
        LibraryType::Strings::iterator it;

        int index = 0;
        FOR_EACH(strings, it) {
            const char* name = it->c_str();
            if(reference_.validForComboList(*ReferenceType(name.c_str()))){
                ItemType parent = tree.rootItem();


				std::string group = library.editorGroup(index);
                Groups::iterator git = groups.find(group);
                if(git != groups.end())
                    parent = git->second;
				else{
					static int temp;
                    parent = groups[group] = tree.addObject(temp, group.c_str(), parent);
				}

                ItemType item = tree.addTreeObjectFiltered(*it, name.c_str(), parent);
				if(strcmp(reference_.c_str(), name.c_str()) == 0)
					result_ = item;
            }
            ++index;
        }

        Groups::iterator git;
		FOR_EACH(groups, git)
			if(git->second != TLI_ROOT && tree.GetChildItem(git->second) == 0)
				tree.deleteItem(git->second);
		return result_;
    }
	bool select(TreeCtrl& tree, ItemType item){
		ItemType parent = tree.GetParentItem(item);
		typedef typename LibraryType::StringType String;
		if(String* str = tree.objectByItem(item)->get<String*>()){
			reference_ = ReferenceType(str->c_str());
			return true;
		}
		else
			return false;
	}
};

typedef EffectReference ReferenceType;
class TreeEffectReferenceSelector : public TreeEditorImpl<TreeEffectReferenceSelector, ReferenceType>{
public:
	bool getLibraryBookmark(LibraryBookmark* bookmark){
		if(EffectContainerLibrary::instance().exists(getData().c_str())){
			if(bookmark)
				*bookmark = LibraryBookmark(EffectContainerLibrary::instance().editName(), getData().c_str(), ComboStrings());
			return true;
		}
		else
			return false;
	}
	bool invokeEditor(ReferenceType& reference, HWND parent);

	bool hideContent() const { return true; }
	Icon buttonIcon() const{ return TreeEditor::ICON_REFERENCE; }
	std::string nodeValue() const { 
		return getData().c_str();
	}
	bool setNodeValue(const char* newValue){
		return false;
	}
};

bool TreeEffectReferenceSelector::invokeEditor (ReferenceType& reference, HWND parent)
{
	CTreeSelectorDlg dlg(CWnd::FromHandle(parent));
	ReferenceTreeBuilder<ReferenceType> treeBuilder(reference);
	dlg.setBuilder(&treeBuilder);
    dlg.DoModal();
    return true;
}
