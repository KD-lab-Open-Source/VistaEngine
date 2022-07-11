#include "Stdafx.h"
#include "TextDB.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "GameOptions.h"

WRAP_LIBRARY(TextIdMap, "TextIdMap", "TextIdMap", "Scripts\\Content\\TextIdMap", 0, false);
WRAP_LOC_LIBRARY(TextDB, "TextDB", "TextDB", GameOptions::instance().getLanguage(), "Text\\Texts.tdb", 0);

///////////////////////////////////////////////
TextDB::TextDB()
{
	saveTextOnly_ = true;
}

bool TextDB::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(static_cast<StaticMap<string, string>&>(*this), name, nameAlt);
}

const char* TextDB::getText(const char* text_id) 
{
	TextDB::const_iterator it = (*this).find(text_id);
	if(it != (*this).end())
		return it->second.c_str();

	(*this)[text_id] = text_id;

	return text_id;
}

bool TextDB::load(const char* filename) 
{
	XPrmIArchive ia;
	if(ia.open(filename)){												
		serialize(ia, "TextDB", "");
		return true;
	}				
	return false;
}

void TextDB::save(const char* filename) 
{
	XPrmOArchive oa(filename);
	serialize(oa, "TextDB", "");
}

///////////////////////////////////////////////

TextIdMap::TextIdMap()
{
	lastId_ = 0;
}

TextIdMap::~TextIdMap()
{
	saveLibrary();
}

void TextIdMap::serialize(Archive& ar)
{
	ar.serialize(lastId_, "lastId", 0);
	if(ar.isOutput())
		map_.serialize(ar, "map", 0);
}

bool TextIdMap::checkId(const char* key, int& id, bool afterEditing)
{
	if(id){
		if(lastId_ <= id)
			lastId_ = id + 1;
		Map::iterator i;
		FOR_EACH(map_, i)
			if(i->second == id){
				if(i->first == key)
					return true;
				if(afterEditing){
					map_.erase(i);
					return checkId(key, id, true);
				}
				else
					id = 0;
			}
	}

	Map::iterator i = map_.find(key);
	if(i == map_.end()){
		if(!id)
			id = ++lastId_;
		map_[key] = id;
		return true;
	}
	else if(afterEditing){
		if(i->second != id)
			return false;
		else if(!id)
			id = ++lastId_;
	}
	return true;
}


