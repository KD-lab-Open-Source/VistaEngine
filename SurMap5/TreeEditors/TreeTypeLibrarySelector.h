#include "EditArchive.h"

#include "..\LibraryBookmark.h"

#include "TreeEditor.h"
#include "TypeLibrary.h"
#include "TreeSelector.h"
#include "ReferenceTreeBuilder.h"


template<class ReferenceType>
class TreeReferenceSelector : public TreeEditorImpl<TreeReferenceSelector<ReferenceType>, ReferenceType>{
public:
	bool getLibraryBookmark(LibraryBookmark* bookmark){
		if(ReferenceType::StringTableType::instance().exists(getData().c_str())){
			if(bookmark){
				const char* libraryName = ReferenceType::StringTableType::instance().name();
				*bookmark = LibraryBookmark(libraryName, getData().c_str(), ComboStrings());
			}
			return true;
		}
		else
			return false;
	}
	bool invokeEditor(ReferenceType& reference, HWND parent);

	bool hideContent() const { return true; }
	Icon buttonIcon() const{
		return TreeEditor::ICON_REFERENCE;
	}
	std::string nodeValue() const { 
		return getData().c_str();
	}

};

template<class ReferenceType>
bool TreeReferenceSelector<ReferenceType>::invokeEditor (ReferenceType& reference, HWND parent)
{
	CTreeSelectorDlg dlg(CWnd::FromHandle(parent));
	ReferenceTreeBuilder<ReferenceType> treeBuilder(reference);
	dlg.setBuilder(&treeBuilder);
    dlg.DoModal();
    return true;
}
