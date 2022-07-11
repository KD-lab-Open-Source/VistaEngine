#ifndef __RESOURCE_SELECTOR_H_INCLUDED__
#define __RESOURCE_SELECTOR_H_INCLUDED__

#include <string>
#include "..\Util\Serialization\Serialization.h"

struct ResourceSelector {
	struct Options {
		Options(const char* _filter, const char* _initialDir, const char* _title, bool _copy = true)
			: filter (_filter) , initialDir (_initialDir) , title (_title) , copy (_copy)
		{
        }
		void serialize (Archive& ar) {
            ar.serialize(filter, "filter", 0);
            ar.serialize(initialDir, "initialDir", 0);
            ar.serialize(title, "title", 0);
            ar.serialize(copy, "copy", 0);
        }
		std::string filter;
        std::string initialDir;
        std::string title;
        bool copy;
	};

	ResourceSelector ()
		: fileNamePtr_ (0)
		, options_ (ResourceSelector::DEFAULT_OPTIONS)
	{
	}
    explicit ResourceSelector (std::string& fileName, const Options& options = DEFAULT_OPTIONS)
        : fileNamePtr_ (&fileName), options_ (options)
    {
    }

    operator const char* () const {
        return fileName_.c_str ();
    }

	Options& options () {
		return options_;
	}

	const Options& options () const {
		return options_;
	}

    bool serialize(Archive& ar, const char* name, const char* nameAlt) {
        if (ar.isEdit ()) {
            bool nodeExists = ar.openStruct (name, nameAlt, typeid(ResourceSelector).name());
			
			if (fileNamePtr_)
	            ar.serialize(*fileNamePtr_, "fileName", "ָלÿ פאיכא");
			else
	            ar.serialize(fileName_, "fileName", "ָלÿ פאיכא");
            ar.serialize(options_, "options_", 0);

            ar.closeStruct (name);
			return nodeExists;
        } else {
			xassert (fileNamePtr_);
            return ar.serialize (*fileNamePtr_, name, nameAlt);
        }
    }

	void setFileName (const char* fileName) {
		fileName_ = fileName;
	}

	static Options DEFAULT_OPTIONS;
	static Options TEXTURE_OPTIONS;
	static Options UI_TEXTURE_OPTIONS;
	static Options FX_TEXTURE_OPTIONS;
	static Options AVI_OPTIONS;
	static Options TGA_AVI_OPTIONS;
	static Options BINK_OPTIONS;
	static Options SOUND_TRACK_OPTIONS;
	static Options TRIGGER_OPTIONS;
	static Options MISSION_OPTIONS;
protected:
    Options options_;
	std::string* fileNamePtr_;
	std::string fileName_;
};

struct ModelSelector : ResourceSelector {
	ModelSelector ()
		: ResourceSelector ()
	{
		fileNamePtr_ = 0;
	}
	ModelSelector (std::string& fileName, const Options& options = DEFAULT_OPTIONS)
		: ResourceSelector(fileName, options)
	{}
    operator const char* () const {
        return fileName_.c_str ();
    }
    bool serialize(Archive& ar, const char* name, const char* nameAlt) {
        if (ar.isEdit ()) {
            bool nodeExists = ar.openStruct (name, nameAlt, typeid(ModelSelector).name());
			
			if (fileNamePtr_)
	            ar.serialize(*fileNamePtr_, "fileName", "ָלÿ פאיכא");
			else
	            ar.serialize(fileName_, "fileName", "ָלÿ פאיכא");
            ar.serialize(options_, "options_", 0);

            ar.closeStruct (name);
			return nodeExists;
        } else {
			xassert (fileNamePtr_);
            return ar.serialize (*fileNamePtr_, name, nameAlt);
        }
    }

	static Options SKY_OPTIONS;
	static Options EFFECT_OPTIONS;
	static Options HEAD_OPTIONS;
	static Options BALMER3DX_OPTIONS;
	static Options DEFAULT_OPTIONS;
};

template<ResourceSelector::Options * Type = &ResourceSelector::DEFAULT_OPTIONS>
struct ResourceSelectorTypified : public ResourceSelector {
	ResourceSelectorTypified() : ResourceSelector()
	{
		options_ = *Type;
	}
    
	bool serialize(Archive& ar, const char* name, const char* nameAlt){
        if(ar.isEdit()){
            bool nodeExists = ar.openStruct (name, nameAlt, typeid(ResourceSelector).name());
			
            ar.serialize(fileName_, "fileName", "ָלÿ פאיכא");
            ar.serialize(options_, "options_", 0);

            ar.closeStruct (name);
			return nodeExists;
        } else
            return ar.serialize (fileName_, name, nameAlt);

    }

};

__declspec(selectany) ResourceSelector::Options ResourceSelector::DEFAULT_OPTIONS ("*.*", ".\\RESOURCE\\", "Will select ANY file", false);
__declspec(selectany) ResourceSelector::Options ResourceSelector::TRIGGER_OPTIONS ("*.scr", "Scripts\\Content\\Triggers", "Trigger Chain Name", false);
__declspec(selectany) ResourceSelector::Options ResourceSelector::TEXTURE_OPTIONS ("*.tga", ".\\Resource\\TerrainData\\Textures", "Will select location of an file textures", true);
__declspec(selectany) ResourceSelector::Options ResourceSelector::FX_TEXTURE_OPTIONS ("*.tga", ".\\Resource\\FX\\Textures", "Will select location of an file textures", true);
__declspec(selectany) ResourceSelector::Options ResourceSelector::AVI_OPTIONS ("*.avi", "Resource\\TerrainData\\Textures", "Will select location of an file textures");
__declspec(selectany) ResourceSelector::Options ResourceSelector::TGA_AVI_OPTIONS ("*.tga; *.avi", "Resource\\TerrainData\\Textures", "Will select location of an file textures");
__declspec(selectany) ResourceSelector::Options ResourceSelector::BINK_OPTIONS ("*.bik", "Resource\\Video", "Will select location of video file");
__declspec(selectany) ResourceSelector::Options ResourceSelector::MISSION_OPTIONS ("*.spg; *.rpl", "Resource\\Worlds", "Will select location of mission file", false);
__declspec(selectany) ResourceSelector::Options ResourceSelector::SOUND_TRACK_OPTIONS ("*.ogg", "Resource\\Music", "Will select location of music file");
__declspec(selectany) ResourceSelector::Options ResourceSelector::UI_TEXTURE_OPTIONS ("*.tga; *.dds; *.avi", "Resource\\UI\\Textures", "Will select location of texture");
__declspec(selectany) ModelSelector::Options    ModelSelector::SKY_OPTIONS ("*.3dx", ".\\Resource\\TerrainData\\Sky", "Will select location of 3DX model");
__declspec(selectany) ModelSelector::Options    ModelSelector::DEFAULT_OPTIONS ("*.3dx", ".\\Resource\\Models", "Will select location of 3DX model");
__declspec(selectany) ModelSelector::Options    ModelSelector::HEAD_OPTIONS ("*.3dx", ".\\Resource\\UI\\Models", "Will select location of 3DX model");
__declspec(selectany) ModelSelector::Options    ModelSelector::BALMER3DX_OPTIONS ("*.3dx", "Scripts\\resource\\balmer" , "Will select location of 3DX model");
__declspec(selectany) ModelSelector::Options    ModelSelector::EFFECT_OPTIONS ("*.effect", ".\\Resource\\Fx", "Will select effect location");

#endif
