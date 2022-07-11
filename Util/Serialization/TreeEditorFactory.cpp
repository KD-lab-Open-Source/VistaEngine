#include "StdAfx.h"
#include "TreeEditor.h"
#include "LibraryWrapper.h"

void TreeEditorFactory::add(const char* dataTypeName, CreatorBase& creator_op, LibraryInstanceFunc func)
{
    inherited::add(dataTypeName, creator_op);
    if(func)
        libraryInstances_[dataTypeName] = func;
}

const char* TreeEditorFactory::findReferencedLibrary(const char* referenceTypeName) const
{
    LibraryNames::const_iterator it = libraryNames_.find(referenceTypeName);
    if(it == libraryNames_.end())
        return "";
    else
        return it->second;
}

void TreeEditorFactory::buildMap()
{
    if(libraryNames_.size() != libraryInstances_.size()){
        LibraryInstances::iterator it;
        FOR_EACH(libraryInstances_, it){
			LibraryInstanceFunc inst = it->second;
            EditorLibraryInterface& library = inst();
            libraryNames_[it->first] = library.name();
        }
    }
}
