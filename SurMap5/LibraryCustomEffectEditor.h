#ifndef __LIBRARY_CUSTOM_EFFECT_EDITOR_H_INCLUDED__
#define __LIBRARY_CUSTOM_EFFECT_EDITOR_H_INCLUDED__

#include "LibraryTreeObject.h"

class LibraryEffectGroupTreeObject : public LibraryGroupTreeObject{
public:
	LibraryEffectGroupTreeObject(LibraryCustomEditor* customEditor, const char* groupName)
	: LibraryGroupTreeObject(customEditor, groupName) {}
	void onMenuCreate();
};

class LibraryCustomEffectEditor: public LibraryCustomEditor{
public:
	LibraryCustomEffectEditor(EditorLibraryInterface* library);
	// virtuals:
	LibraryGroupTreeObject* createGroupTreeObject(const char* groupName);

	// ^^^
protected:

};

#endif
