#ifndef __TEXTDB_H__
#define __TEXTDB_H__

#include "StaticMap.h"
#include "LibraryWrapper.h"

class TextDB : public StaticMap<std::string, std::string>, public LocLibraryWrapper<TextDB>
{
public:
	TextDB();

	const char* getText(const char* text_id);
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	
	bool load(const char* filename);
	void save(const char* filename);

private:
	typedef StaticMap<std::string, std::string> Map;
};

class TextIdMap : public LibraryWrapper<TextIdMap>
{
public:
	typedef StaticMap<string, int> Map;

	TextIdMap();
	~TextIdMap();
	bool checkId(const char* key, int& id, bool afterEditing);
	void serialize(Archive& ar);

	const Map& map() const { return map_; }
	void setSave(bool save) { save_ = save; }
private:
	int lastId_;
	bool save_;
	Map map_;
};

#endif //__TEXTDB_H__
