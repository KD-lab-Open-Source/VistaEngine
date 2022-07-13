#include "StdAfx.h"

#include "EffectReference.h"
#include "TreeEditor.h"
#include "EditArchive.h"
#include "ResourceSelector.h"
#include "FileUtils.h"

typedef StringTableBasePolymorphic<EffectContainer> EffectContainerElement;

std::string selectAndCopyResource(const char* internalResourcePath, const char* filter, const char* defaultName, const char* title);

class TreeEffectContainerEditor : public TreeEditorImpl<TreeEffectContainerEditor, EffectContainerElement> {
public:
    bool invokeEditor(EffectContainerElement& effect, HWND wnd)
	{
		ModelSelector::Options& options = ModelSelector::EFFECT_OPTIONS;
		
        string request = selectAndCopyResource(options.initialDir.c_str(), options.filter.c_str(), "", options.title.c_str());
        if(!request.empty()){
			if(!effect.get())
				effect.set(new EffectContainer);
			effect.get()->setFileName(request.c_str());
			effect.setName(extractFileBase(request.c_str()).c_str());
            return true;
        }
	    return false;
    }
    bool hideContent () const{ return false; }

	Icon buttonIcon()const { return TreeEditor::ICON_FILE; }

	std::string nodeValue () const{ 
		return getData().get() ? getData().get()->fileName() : "";
	}

};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(EffectContainerElement).name(), TreeEffectContainerEditor);
