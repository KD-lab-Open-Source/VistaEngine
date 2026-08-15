// WorldList.h — the engine-touching half of the world selection dialog.
//
// Compiled in EditorEngine: it uses MissionDescriptions (Network/NetPlayer.h)
// to scan a worlds directory the way CDlgSelectWorld::fillWorldList did, and
// hands the Qt side plain, engine-free rows (name, interface name, size).
// Qt translation units must never include engine headers, so this is the only
// surface they see.
#pragma once

#include <string>
#include <vector>

class WorldList
{
public:
	// One row of the world list (CDlgSelectWorld's three columns).
	struct World
	{
		std::string name;         // worldName()
		std::string interfaceName;// interfaceName()
		int sizeX = 0;            // round(worldSize().x)
		int sizeY = 0;            // round(worldSize().y)
	};

	// Scan path2worlds. `userWorlds` picks readUserWorldsFromDir over
	// readFromDir(..., GAME_TYPE_SCENARIO), mirroring the #ifdef in the
	// original fillWorldList.
	bool scan(const std::string& path2worlds, bool userWorlds);

	const std::vector<World>& worlds() const { return worlds_; }

	// Delete a world and its mission files, as CDlgSelectWorld::OnButtonDelete.
	static bool deleteWorld(const std::string& path2worlds, const std::string& name);

	// Rename a world's files, as renameWorldVerbose() in DlgSelectWorld.cpp.
	static bool renameWorld(const std::string& path2worlds,
	                        const std::string& oldName,
	                        const std::string& newName);

	// Create the world directory; returns false if it already exists/fails
	// (CDlgSelectWorld::OnButtonNew).
	static bool createWorldDir(const std::string& path2worlds, const std::string& name);

private:
	std::vector<World> worlds_;
};
