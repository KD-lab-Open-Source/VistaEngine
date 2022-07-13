#include "StdAfx.h"
#include "LibraryTreeObject.h"
#include "LibraryEditorTree.h"
#include "LibraryTab.h"
#include "Parameters.h"
#include "mfc\PopupMenu.h"
#include "Dictionary.h"

class LibraryCustomParameterEditor : public LibraryCustomEditor{
public:
	LibraryCustomParameterEditor(EditorLibraryInterface* library);
	LibraryGroupTreeObject* createGroupTreeObject(const char* groupName);
};

REGISTER_LIBRARY_CUSTOM_EDITOR(&ParameterValueTable::instance, LibraryCustomParameterEditor);
// ---------------------------------------------------------------------------

class LibraryParameterGroupTreeObject : public LibraryGroupTreeObject{
	typedef LibraryParameterGroupTreeObject Self;
public:
	LibraryParameterGroupTreeObject(LibraryCustomEditor* customEditor, const char* groupName);

	void onMenuConstruction(PopupMenuItem& root);

	void onMenuRemoveGroup();
	void onMenuRemoveGroupRecursively();

	bool onBeginDrag();
	bool onDragOver(const TreeObjects& objects);
	void onDrop(const TreeObjects& object);
};

// ---------------------------------------------------------------------------

LibraryParameterGroupTreeObject::LibraryParameterGroupTreeObject(LibraryCustomEditor* customEditor, const char* groupName)
: LibraryGroupTreeObject(customEditor, groupName)
{
	
}

void LibraryParameterGroupTreeObject::onMenuConstruction(PopupMenuItem& root)
{
	if(groupName_ == ""){
		if(!root.empty())
			root.addSeparator();

		root.add(TRANSLATE("Удалить все параметры без группы"))
			.connect(bindMethod(*this, &Self::onMenuRemoveGroupRecursively));
	}
	else{
		if(!root.empty())
			root.addSeparator();

		root.add(TRANSLATE("Удалить группу"))
			.connect(bindMethod(*this, &Self::onMenuRemoveGroup));
		root.add(TRANSLATE("Удалить группу со всем параметрами"))
			.connect(bindMethod(*this, &Self::onMenuRemoveGroupRecursively));
	}
}

void LibraryParameterGroupTreeObject::onMenuRemoveGroup()
{
	EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
	xassert(groups);
	groups->editorElementErase(groupName_.c_str());

	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
	LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
	// TODO focus
}

void LibraryParameterGroupTreeObject::onMenuRemoveGroupRecursively()
{
	xassert(customEditor_);
	EditorLibraryInterface* library = customEditor_->library();
	xassert(library);
	for(int i = 0; i < library->editorSize(); ){
		if(library->editorElementGroup(i) == groupName_)
			library->editorElementErase(i);
		else
			++i;
	}
	
	EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
	xassert(groups);
	if(groupName_ != "")
		groups->editorElementErase(groupName_.c_str());

	CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);
	LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
}

bool LibraryParameterGroupTreeObject::onBeginDrag()
{
	return groupName_ != "";
}

bool LibraryParameterGroupTreeObject::onDragOver(const TreeObjects& objects)
{
	xassert(!objects.empty());
	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(objects.front());
	if(object->isGroup()){
		return true;
	}
	else
		return LibraryGroupTreeObject::onDragOver(objects);
}

void LibraryParameterGroupTreeObject::onDrop(const TreeObjects& objects)
{	
	xassert(!objects.empty());
	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(objects.front());
	xassert(customEditor_);
	EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
	if(object->isGroup()){
		LibraryGroupTreeObject* groupObject = safe_cast<LibraryGroupTreeObject*>(object);
		int index = groups->editorFindElement(groupObject->groupName());
		int currentIndex = groups->editorFindElement(groupName_.c_str());
		groups->editorElementMoveBefore(index, currentIndex);

		std::string groupName = groupObject->groupName();
		TreeObject* rootObject = tree_->rootObject();
		LibraryTabEditable::buildLibraryTree(tree_->rootObject(), customEditor_); // suicide
		focusObjectByGroupName(groupName.c_str(), rootObject);
	}
	else
		LibraryGroupTreeObject::onDrop(objects);
}

// ---------------------------------------------------------------------------


LibraryCustomParameterEditor::LibraryCustomParameterEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &ParameterValueTable::instance());
}

LibraryGroupTreeObject* LibraryCustomParameterEditor::createGroupTreeObject(const char* groupName)
{
	return new LibraryParameterGroupTreeObject(this, groupName);
}

