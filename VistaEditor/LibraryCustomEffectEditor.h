#ifndef __LIBRARY_CUSTOM_EFFECT_EDITOR_H_INCLUDED__
#define __LIBRARY_CUSTOM_EFFECT_EDITOR_H_INCLUDED__

#include "kdw/LibraryTreeObject.h"

class LibraryEffectGroupTreeObject : public kdw::LibraryGroupTreeObject{
public:
	LibraryEffectGroupTreeObject(kdw::LibraryCustomEditor* customEditor, const char* groupName)
	: LibraryGroupTreeObject(customEditor, groupName) {}
	void onMenuCreate(kdw::LibraryTree* tree);
};

class LibraryCustomEffectEditor: public kdw::LibraryCustomEditor{
public:
	LibraryCustomEffectEditor(EditorLibraryInterface* library);
	// virtuals:
	kdw::LibraryGroupTreeObject* createGroupTreeObject(const char* groupName);

	// ^^^
protected:

};

#endif
