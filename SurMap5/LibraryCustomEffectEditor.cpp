#include "StdAfx.h"
#include "LibraryCustomEffectEditor.h"
#include "LibraryEditorTree.h"
#include "ResourceSelector.h"
#include "LibraryWrapper.h"
#include "FileUtils.h"
#include "EffectReference.h"
#include "LibraryTab.h"
string selectAndCopyModel(const char* resourceHomePath, const char* filter, const char* defaultName, const char* title);

LibraryGroupTreeObject* LibraryCustomEffectEditor::createGroupTreeObject(const char* groupName)
{
	xassert(library_);
	return new LibraryEffectGroupTreeObject(this, groupName);
}

LibraryCustomEffectEditor::LibraryCustomEffectEditor(EditorLibraryInterface* library)
: LibraryCustomEditor(library)
{
	xassert(library == &EffectLibrary::instance());
}

std::string makeName(const char* reservedComboList, const char* nameBase);

void LibraryEffectGroupTreeObject::onMenuCreate()
{
	if(customEditor_){
		CLibraryEditorTree* tree = safe_cast<CLibraryEditorTree*>(tree_);

		std::string groupName = fullGroupName();

		ModelSelector::Options& options = ModelSelector::EFFECT_OPTIONS;
		string request = selectAndCopyModel(options.initialDir.c_str(), options.filter.c_str(), "", options.title.c_str());

		if(request.empty())
			return;

		std::string name = extractFileBase(request.c_str());
		const char* comboList = customEditor_->library()->editorComboList();
		name = makeName(comboList, name.c_str());
		name = customEditor_->library()->editorAddElement(name.c_str(), groupName.c_str());

		EffectReference ref(name.c_str());
		xassert(&*ref);
		EffectContainer& container = const_cast<EffectContainer&>(*ref);
		container.setFileName(request.c_str());

		LibraryTabEditable::buildLibraryTree(tree->rootObject(), customEditor_); // suicide
		if(LibraryElementTreeObject* object = safe_cast<LibraryElementTreeObject*>(objectByElementName(name.c_str(), "", tree->rootObject())))
			object->focus();
		else
			xassert(0);

		tree->UpdateWindow();
	}
}


REGISTER_LIBRARY_CUSTOM_EDITOR(&EffectLibrary::instance, LibraryCustomEffectEditor);