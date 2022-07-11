#ifndef __LIBRARIES_MANAGER_H_INCLUDED__
#define __LIBRARIES_MANAGER_H_INCLUDED__

#include "StaticMap.h"

class EditorLibraryInterface;
class LibraryWrapperBase;

class LibrariesManager{
public:
    static LibrariesManager& instance();

    typedef EditorLibraryInterface&(*LibraryInstanceFunc)(void);
    typedef LibraryWrapperBase&(*DummyInstanceFunc)(void);

    bool registerLibrary(const char* name, LibraryInstanceFunc func, bool editable);

    LibraryInstanceFunc findInstanceFunc(const char* name);
	EditorLibraryInterface* find(const char* name);

    typedef StaticMap<std::string, LibraryInstanceFunc> Libraries;

    Libraries& libraries() { return libraries_; }
    Libraries& editorLibraries() { return editorLibraries_; }
private:
    LibrariesManager();
    Libraries libraries_;
	Libraries editorLibraries_;
};

#endif
