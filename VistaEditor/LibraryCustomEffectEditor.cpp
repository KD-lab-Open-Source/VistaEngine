#include "StdAfx.h"
#include "LibraryCustomEffectEditor.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\LibraryWrapper.h"
#include "Serialization\StringTableBase.h"
#include "Serialization\StringTable.h"
#include "Serialization\SerializationFactory.h"
#include "FileUtils\FileUtils.h"
#include "kdw/LibraryTree.h"
#include "kdw/ContentUtil.h"
#include "kdw/LibraryTab.h"
#include "EffectReference.h"

DECLARE_SEGMENT(LibraryCustomEffectEditor)
REGISTER_LIBRARY_CUSTOM_EDITOR(&EffectLibrary::instance, LibraryCustomEffectEditor);

kdw::LibraryGroupTreeObject* LibraryCustomEffectEditor::createGroupTreeObject(const char* groupName)
{
	xassert(library_);
	return new LibraryEffectGroupTreeObject(this, groupName);
}

LibraryCustomEffectEditor::LibraryCustomEffectEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &EffectLibrary::instance());
}

void LibraryEffectGroupTreeObject::onMenuCreate(kdw::LibraryTree* tree)
{
	if(customEditor_){
		std::string groupName = fullGroupName();

		static ModelSelector::Options options("*.effect", "Resource\\Fx", "Will select effect location");
		static const char* filter[] = { "*.effect", "*.effect", 0 };
		string request = kdw::selectAndCopyModel(options.initialDir.c_str(), filter, "", options.title.c_str());

		if(request.empty())
			return;

		std::string name = extractFileBase(request.c_str());
		const char* comboList = customEditor_->library()->editorComboList();
		name = kdw::makeName(comboList, name.c_str());
		name = customEditor_->library()->editorAddElement(name.c_str(), groupName.c_str());

		EffectReference ref(name.c_str());
		xassert(&*ref);
		EffectContainer& container = const_cast<EffectContainer&>(*ref);
		container.setFileName(request.c_str());

		kdw::LibraryTabEditable::buildLibraryTree(tree, tree->root(), customEditor_); // suicide
		if(kdw::LibraryElementTreeObject* object = safe_cast<kdw::LibraryElementTreeObject*>(objectByElementName(name.c_str(), "", tree->root())))
			object->focus();
		else
			xassert(0);

		//tree->UpdateWindow();
	}
}


