#include "stdafx.h"
#include "LibraryAttribEditor.h"
#include "AttribEditor\TreeListCtrl\xTreeCtrlLib.h"
#include "Serialization\Dictionary.h"
#include "Serialization\LibraryBookmark.h"
#include "kdw/PropertyRow.h"
#include "kdw/PropertyTree.h"
#include "kdw/PropertyTreeModel.h"
#include "mfc\PopupMenu.h"

CLibraryAttribEditor::CLibraryAttribEditor()
{
}


int CLibraryAttribEditor::initControl()
{
	int result = __super::initControl();
	tree().signalSearch().connect(this, &CLibraryAttribEditor::onSearch);
	tree().signalFollowReference().connect(this, &CLibraryAttribEditor::onFollowReference);
	tree().setHasLibrarySupport(true);
	return result;
}

void CLibraryAttribEditor::onChanged()
{
	if(signalElementChanged_)
		signalElementChanged_();
}

void CLibraryAttribEditor::onSelected()
{
	if(signalElementSelected_){
		kdw::PropertyTreeModel* model = tree().model();
		kdw::TreeModel::Selection selection = model->selection();
		selectedPath_.clear();
		if(!selection.empty())
			getItemPath(selectedPath_, safe_cast<ItemType>(model->rowFromPath(selection.front())));
		signalElementSelected_(selectedPath_);	
	}
}

void CLibraryAttribEditor::onSearch(ItemType item)
{
	if(signalSearchInvoked_)
		signalSearchInvoked_(item);
}

void CLibraryAttribEditor::onFollowReference(LibraryBookmark bookmark)
{
	if(signalFollowReference_)
		signalFollowReference_(bookmark);
}

