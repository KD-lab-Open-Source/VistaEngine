#ifndef __SOUND_TRACK_H__
#define __SOUND_TRACK_H__

#include "Serialization\StringTableReference.h"
#include "Serialization\StringTableBase.h"

class SoundTrack : public StringTableBase
{
public:
	SoundTrack(const char* theme = "") : StringTableBase(theme) { randomChoice = 0; index = 0; }

	const char* fileName() const;
	void nextFileName() const;

	void serialize(Archive& ar);

private:
	struct Name
	{
		Name(const char* nameIn = "") : fileName_(nameIn) {}
		operator const char*() const { return fileName_.c_str(); }
		bool serialize(Archive& ar, const char* name, const char* nameAlt);

		string fileName_;
	};

	vector<Name> fileNames; 
	bool randomChoice;
	mutable int index;
};

typedef StringTable<SoundTrack> SoundTrackTable;
typedef StringTableReference<SoundTrack, true> SoundTrackReference;

#endif //__SOUND_TRACK_H__
