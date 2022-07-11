#include "stdafx.h"
#include "LibraryWrapper.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "EditArchive.h"
#include "LibrariesManager.h"

LibraryWrapperBase::LibraryWrapperBase()
{
	crc_ = 0;
	saveTextOnly_ = false;
}

void LibraryWrapperBase::loadLibrary() 
{
	MultiIArchive ia;													
	if(ia.open(fileName_.c_str(), "..\\ContentBin", "")){												
		crc_ = ia.crc();
		int versionOld = 0;												
		ia.serialize(versionOld, "Version", 0);							
		ia.setVersion(versionOld);										
		serializeLibrary(ia.archive());								
		if(versionOld != version_)										
			saveLibrary();											
	}																	
	if(check_command_line(sectionName_)){								
		editLibrary();											
		ErrH.Exit();													
	}	
}

void LibraryWrapperBase::saveLibrary() 
{
	if(!saveTextOnly_){
		MultiOArchive oa(fileName_.c_str(), "..\\ContentBin", "");											
		oa.serialize(version_, "Version", 0);							
		serializeLibrary(oa.xprmArchive());									
		//serializeLibrary(oa.binaryArchive());									
	}
	else{
		XPrmOArchive oa(fileName_.c_str());
		oa.serialize(version_, "Version", 0);							
		serializeLibrary(oa);
	}
}

bool LibraryWrapperBase::editLibrary(bool translatedOnly) 
{
	string setupName = string("Scripts\\TreeControlSetups\\") + sectionName_;
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, setupName.c_str()));
	ea.setTranslatedOnly(translatedOnly);										
	serializeLibrary(static_cast<EditOArchive&>(ea));
	if(ea.edit()){
		serializeLibrary(static_cast<EditIArchive&>(ea));
		saveLibrary();
		return true;
	}

	return false;														
}


LocLibraryWrapperBase::LocLibraryWrapperBase() : LibraryWrapperBase()
{
	file_ = 0;
	xassert(!locDataRootPath().empty() && "Не установлен путь к LocData!");
}

string& LocLibraryWrapperBase::locDataRootPath()
{
	static string path;
	return path;
}

bool LibrariesManager::registerLibrary(const char* name, LibraryInstanceFunc func, bool editor)
{
	if(editor){
		editorLibraries_[name] = func;
	}
	libraries_[name] = func;
	return true;
}

void LocLibraryWrapperBase::loadLanguage(const char* language)
{
	if(language_ != language){
		language_ = language;
		fileName_ = locDataRootPath() + "\\" + language_ + "\\" + file_;
		loadLibrary();
	}
}
LibrariesManager::LibrariesManager()
{
}

LibrariesManager::LibraryInstanceFunc LibrariesManager::findInstanceFunc(const char* name)
{
    Libraries::iterator it = libraries_.find(name);
    if(it == libraries_.end())
        return 0;
    else
        return it->second;
}

EditorLibraryInterface* LibrariesManager::find(const char* name)
{
    Libraries::iterator it = libraries_.find(name);
    if(it == libraries_.end())
		return 0;
	else
		return &it->second();
}

LibrariesManager& LibrariesManager::instance()
{
    static LibrariesManager the;
    return the;
}

void EditorLibraryInterface::editorElementErase(const char* name)
{
	int index = editorFindElement(name);
	xassert(index >= 0 && index < editorSize());
	editorElementErase(index);
}

void EditorLibraryInterface::editorElementMoveBefore(const char* name, const char* beforeName)
{
	int index = editorFindElement(name);
	int beforeIndex = editorFindElement(beforeName);
	xassert(index >= 0 && index < editorSize());
	xassert(beforeIndex >= 0 && beforeIndex < editorSize());
    
	editorElementMoveBefore(index, beforeIndex);
}

void EditorLibraryInterface::editorElementSetName(const char* name, const char* newName)
{
	int index = editorFindElement(name);
	xassert(index >= 0 && index < editorSize());
	editorElementSetName(index, newName);
}

Serializeable EditorLibraryInterface::editorElementSerializeable(const char* editorName, const char* name, const char* nameAlt, bool protectedName)
{
	int index = editorFindElement(editorName);
	xassert(index >= 0 && index < editorSize());
    return editorElementSerializeable(index, name, nameAlt, protectedName);
}

void EditorLibraryInterface::editorElementSetGroup(const char* name, const char* group)
{
	int index = editorFindElement(name);
	xassert(index >= 0 && index < editorSize());
	editorElementSetGroup(index, group);
}

std::string EditorLibraryInterface::editorElementGroup(const char* elementName) const
{
	int index = editorFindElement(elementName);
	xassert(index >= 0 && index < editorSize());
	return editorElementGroup(index);
}

int EditorLibraryInterface::editorFindElement(const char* elementName) const
{
    int count = editorSize();
    for(int i = 0; i < count; ++i)
        if(strcmp(editorElementName(i), elementName) == 0)
            return i;
    return -1;
}
