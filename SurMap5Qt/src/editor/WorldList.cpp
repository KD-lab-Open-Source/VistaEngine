// WorldList.cpp — see header. Compiled with the engine's flags.

#include "WorldList.h"

// The engine's headers assume the old StdAfx preamble (xassert, FOR_EACH,
// using namespace std).
#include <vector>
#include <string>
using namespace std;
#include "xutil.h"
#include "my_STL.h"

// FileTime.h (included via NetPlayer.h) uses Archive& without declaring it —
// the old stdafx.h brought the declaration in. Restore it here.
class Archive;

// NetPlayer.h uses Vect2f/Vect2s; the old stdafx.h included xmath.h explicitly.
#include "XMath/xmath.h"

#include "FileUtils/FileUtils.h"   // DirIterator, isFileExists
#include "Serialization/XPrmArchive.h"
#include "Serialization/Dictionary.h"

// WorldList::scan reads the .spg header files directly. The original dialog
// used MissionDescriptions::readFromDir, but that lives in MissionDescription.cpp
// (the Game executable, which drags Runtime/Universe in). The .spg format is
// XPrm text — a MissionDescription::serialize dump — so the three fields we
// show (interfaceName, worldSize) are read with XPrmIArchive alone.

// openStruct needs a class type (Archive::IsPolymorphic derives from it).
struct HeaderTag {};

bool WorldList::scan(const std::string& path2worlds, bool userWorlds)
{
	worlds_.clear();

	const char* ext = "spg"; // MissionDescription::getExtention: spg for all non-reel

	std::string fixedPath = path2worlds;
	if(!fixedPath.empty() && fixedPath[fixedPath.size() - 1] != '\\')
		fixedPath += "\\";

	const std::string mask = fixedPath + "*." + ext;

	WIN32_FIND_DATA ffd;
	HANDLE hf = FindFirstFile(mask.c_str(), &ffd);
	if(hf == INVALID_HANDLE_VALUE)
		return false;

	do {
		// readFromDir skips zero-size entries.
		if(!ffd.nFileSizeLow)
			continue;

		const std::string fileName = fixedPath + ffd.cFileName;

		World w;
		w.name = ffd.cFileName;
		// Strip the extension for the name column (setInterfaceName drops it).
		{
			size_t dot = w.name.rfind('.');
			if(dot != std::string::npos)
				w.name.erase(dot);
		}

		// The .spg header is a MissionDescription::serialize dump (XPrm text).
		// Read just the two fields the list shows; openNode skips the others.
		XPrmIArchive ia;
		if(!ia.open(fileName.c_str(), 10000))
			continue;
		{
			HeaderTag header;
			if(!ia.openStruct(header, "header", 0)){
				ia.close();
				continue;
			}
			ia.serialize(w.interfaceName, "interfaceName", 0);
			Vect2s worldSize;
			ia.serialize(worldSize, "worldSize", 0);
			w.sizeX = worldSize.x;
			w.sizeY = worldSize.y;
			ia.closeStruct("header");
		}
		ia.close();

		worlds_.push_back(w);
	} while(FindNextFile(hf, &ffd));
	FindClose(hf);

	// Worlds created by the editor (New World: vMap.create+save) have no .spg
	// mission file — only the world directory with world.cls. The original
	// dialog listed .spg missions only, but a freshly made world must be openable
	// too, so list the world directories as a fallback.
	if(worlds_.empty()){
		const std::string dirMask = fixedPath + "*";
		WIN32_FIND_DATA dfd;
		HANDLE dh = FindFirstFile(dirMask.c_str(), &dfd);
		if(dh != INVALID_HANDLE_VALUE){
			do {
				if(!(dfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					continue;
				if(dfd.cFileName[0] == '.')
					continue;   // . and ..
				const std::string worldFile = fixedPath + dfd.cFileName + "\\world.cls";
				if(!isFileExists(worldFile.c_str()))
					continue;
				World w;
				w.name = dfd.cFileName;
				worlds_.push_back(w);
			} while(FindNextFile(dh, &dfd));
			FindClose(dh);
		}
	}

	return !worlds_.empty();
}

bool WorldList::deleteWorld(const std::string& path2worlds, const std::string& name)
{
	// CDlgSelectWorld::OnButtonDelete: vMap::deleteWorld (delete the world
	// directory's files + the dir itself) + remove the .spg mission files.
	// vMap is not pulled in here; its deleteWorld is plain Win32, so it is
	// replicated.
	const std::string path2world = path2worlds + "\\" + name;

	std::string filemask = path2world + "\\*.*";
	WIN32_FIND_DATA ffd;
	HANDLE hf = FindFirstFile(filemask.c_str(), &ffd);
	if(hf != INVALID_HANDLE_VALUE){
		do {
			if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;   // skip . and ..
			DeleteFile((path2world + "\\" + ffd.cFileName).c_str());
		} while(FindNextFile(hf, &ffd));
		FindClose(hf);
	}
	RemoveDirectory(path2world.c_str());

	::DeleteFile((path2worlds + "\\" + name + ".spg").c_str());
	::DeleteFile((path2worlds + "\\" + name + ".spg.bin").c_str());
	return true;
}

bool WorldList::renameWorld(const std::string& path2worlds,
                            const std::string& oldName,
                            const std::string& newName)
{
	// renameWorldVerbose(): rename oldName.* -> newName.* in the worlds dir.
	typedef std::vector<std::pair<std::string, std::string> > Filenames;
	Filenames filenames;

	const std::string prefix = path2worlds + "\\";
	DirIterator it((prefix + oldName + ".*").c_str());
	DirIterator end;
	while(it != end){
		const std::string source = prefix + *it;
		const std::string destination = prefix + newName + (*it + oldName.size());
		if(::isFileExists(destination.c_str()))
			return false;   // target exists
		filenames.push_back(std::make_pair(source, destination));
		++it;
	}

	for(Filenames::iterator fit = filenames.begin(); fit != filenames.end(); ++fit){
		if(!::MoveFile(fit->first.c_str(), fit->second.c_str()))
			return false;
	}
	return true;
}

bool WorldList::createWorldDir(const std::string& path2worlds, const std::string& name)
{
	const std::string dir = path2worlds + "\\" + name;
	return ::CreateDirectory(dir.c_str(), NULL) != FALSE;
}
