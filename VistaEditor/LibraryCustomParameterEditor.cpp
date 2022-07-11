#include "StdAfx.h"
#include "XTL/SafeCast.h"
#include "Serialization\Dictionary.h"
#include "Serialization\BinaryArchive.h"
#include "Serialization\StringTable.h"
#include "kdw/LibraryTreeObject.h"
#include "kdw/LibraryTree.h"
#include "kdw/LibraryTab.h"
#include "kdw/LibraryElementCreateDialog.h"
#include "kdw/PopupMenu.h"
#include "Units/Parameters.h"

class LibraryCustomParameterEditor : public kdw::LibraryCustomEditor{
public:
	LibraryCustomParameterEditor(EditorLibraryInterface* library);
	kdw::LibraryGroupTreeObject* createGroupTreeObject(const char* groupName);
};

DECLARE_SEGMENT(LibraryCustomParameterEditor)
REGISTER_LIBRARY_CUSTOM_EDITOR(&ParameterValueTable::instance, LibraryCustomParameterEditor);
// ---------------------------------------------------------------------------

class LibraryParameterGroupTreeObject : public kdw::LibraryGroupTreeObject{
	typedef LibraryParameterGroupTreeObject Self;
public:
	LibraryParameterGroupTreeObject(kdw::LibraryCustomEditor* customEditor, const char* groupName);

	void onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree);

	void onMenuRemoveGroup(kdw::ObjectsTree* tree);
	void onMenuRemoveGroupRecursively(kdw::ObjectsTree* tree);
	void onMenuRenameGroup(kdw::ObjectsTree* tree);
	void onMenuCopyGroup(kdw::ObjectsTree* tree);

	bool onBeginDrag(kdw::ObjectsTree* tree);
	bool onDragOver(const kdw::TreeObjects& objects, kdw::ObjectsTree* tree);
	void onDrop(const kdw::TreeObjects& object, kdw::ObjectsTree* tree);
};

// ---------------------------------------------------------------------------

LibraryParameterGroupTreeObject::LibraryParameterGroupTreeObject(kdw::LibraryCustomEditor* customEditor,
																 const char* groupName)
: kdw::LibraryGroupTreeObject(customEditor, groupName)
{
	
}

void LibraryParameterGroupTreeObject::onMenuRenameGroup(kdw::ObjectsTree* tree)
{
	
	kdw::LibraryElementCreateDialog dlg(tree, false, false, groupName_.c_str());
	
	if(dlg.showModal() == kdw::RESPONSE_OK){
		EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
		if(groups->editorFindElement(dlg.name()) == -1){
			groups->editorElementSetName(groupName_.c_str(), dlg.name());
			groupName_ = dlg.name();
			setText(dlg.name());
		}
		else{
			MessageBoxA(NULL,"Такoe имя уже существует!!!","Error!!!",MB_ICONERROR|MB_OK);
		}
	}
}

void LibraryParameterGroupTreeObject::onMenuCopyGroup(kdw::ObjectsTree* _tree)
{
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
	xassert(customEditor_);
	EditorLibraryInterface* library = customEditor_->library();
	xassert(library);

	std::string newGroup = kdw::makeName(library->editorGroupsComboList(), groupName_.c_str());
	kdw::LibraryElementCreateDialog dlg(tree, false, false, newGroup.c_str());
	if(dlg.showModal() == kdw::RESPONSE_OK)
	{
		EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
		if(groups->editorFindElement(dlg.name()) == -1)
			newGroup = dlg.name();
		library->editorAddGroup(newGroup.c_str());

		for(int i = 0; i < library->editorSize(); ){
			if(library->editorElementGroup(i) == groupName_){
				std::string newElement = kdw::makeName(library->editorComboList(), library->editorElementName(i));
				if(library->editorFindElement(newElement.c_str()) != -1)
					xassert(0 && "makeName");
				library->editorAddElement(newElement.c_str(), newGroup.c_str());
				Serializer element = library->editorElementSerializer(i, "", "", true);
				BinaryOArchive oa;
				element.serialize(oa);
				BinaryIArchive ia(oa);
				element = library->editorElementSerializer(newElement.c_str(), "", "", true);
				element.serialize(ia);
			}
			++i;
		}

		kdw::LibraryTabEditable::buildLibraryTree(tree, tree->root(), customEditor_); // suicide
		focusObjectByGroupName(newGroup.c_str(), tree->root());
	}
}

void LibraryParameterGroupTreeObject::onMenuConstruction(kdw::PopupMenuItem& root, kdw::ObjectsTree* tree)
{
	if(groupName_ == ""){
		if(!root.empty())
			root.addSeparator();

		root.add(TRANSLATE("Удалить все параметры без группы"),  tree)
			.connect(this, &Self::onMenuRemoveGroupRecursively);
	}
	else{
		if(!root.empty())
			root.addSeparator();

		root.add(TRANSLATE("Удалить группу"), tree)
			.connect(this, &Self::onMenuRemoveGroup);
		root.add(TRANSLATE("Удалить группу со всем параметрами"), tree)
			.connect(this, &Self::onMenuRemoveGroupRecursively);
	}

	root.addSeparator();
	root.add(TRANSLATE("Переименовать"), tree)
		.connect(this, &Self::onMenuRenameGroup);

	root.addSeparator();
	root.add(TRANSLATE("Дублировать вместе с параметрами"), tree)
		.connect(this, &Self::onMenuCopyGroup);

}

void LibraryParameterGroupTreeObject::onMenuRemoveGroup(kdw::ObjectsTree* _tree)
{
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
	EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
	xassert(groups);
	groups->editorElementErase(groupName_.c_str());

	xassert(customEditor_);
	EditorLibraryInterface* library = customEditor_->library();
	xassert(library);
	for(int i = 0; i < library->editorSize(); ){
		if(library->editorElementGroup(i) == groupName_){
			library->editorElementSetGroup(i, "");
		}
		++i;
	}

	kdw::LibraryTabEditable::buildLibraryTree(tree, tree->root(), customEditor_); // suicide
	// TODO focus
}

void LibraryParameterGroupTreeObject::onMenuRemoveGroupRecursively(kdw::ObjectsTree* _tree)
{
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
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

	kdw::LibraryTabEditable::buildLibraryTree(tree, tree->root(), customEditor_); // suicide
}

bool LibraryParameterGroupTreeObject::onBeginDrag(kdw::ObjectsTree* tree)
{
	return groupName_ != "";
}

bool LibraryParameterGroupTreeObject::onDragOver(const kdw::TreeObjects& objects, kdw::ObjectsTree* tree)
{
	xassert(!objects.empty());
	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(objects.front());
	if(object->isGroup()){
		return true;
	}
	else
		return kdw::LibraryGroupTreeObject::onDragOver(objects, tree);
}

void LibraryParameterGroupTreeObject::onDrop(const kdw::TreeObjects& objects, kdw::ObjectsTree* _tree)
{	
	kdw::LibraryTree* tree = safe_cast<kdw::LibraryTree*>(_tree);
	xassert(!objects.empty());
	LibraryTreeObject* object = safe_cast<LibraryTreeObject*>(objects.front());
	xassert(customEditor_);
	EditorLibraryInterface* groups = LibrariesManager::instance().find("ParameterGroup");
	if(object->isGroup()){
		kdw::LibraryGroupTreeObject* groupObject = safe_cast<kdw::LibraryGroupTreeObject*>(object);
		int index = groups->editorFindElement(groupObject->groupName());
		int currentIndex = groups->editorFindElement(groupName_.c_str());
		groups->editorElementMoveBefore(index, currentIndex);

		std::string groupName = groupObject->groupName();
		kdw::LibraryTabEditable::buildLibraryTree(tree, tree->root(), customEditor_); // suicide
		focusObjectByGroupName(groupName.c_str(), tree->root());
	}
	else
		kdw::LibraryGroupTreeObject::onDrop(objects, tree);
}

// ---------------------------------------------------------------------------


LibraryCustomParameterEditor::LibraryCustomParameterEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &ParameterValueTable::instance());
}

kdw::LibraryGroupTreeObject* LibraryCustomParameterEditor::createGroupTreeObject(const char* groupName)
{
	return new LibraryParameterGroupTreeObject(this, groupName);
}

