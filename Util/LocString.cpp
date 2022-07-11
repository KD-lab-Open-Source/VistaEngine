#include "StdAfx.h"
#include "LocString.h"
#include "serialization.h"
#include "TextDB.h"

LocString::LocString()
: id_(0)
{}

void LocString::serialize(Archive& ar)
{
	ar.serialize(key_, "key", "&Ключ");
	ar.serialize(id_, "id", 0);

	if(ar.isInput()){
		if(!key_.empty()){
			if(!TextIdMap::instance().checkId(key_.c_str(), id_, ar.isEdit())){
				xassertStr(0 && "Ключ с таким именем уже есть", key_.c_str());
				//key_ += "_";
			}
		}

		update();
	}
}

void LocString::update()
{
	text_ = TextDB::instance().getText(key_.c_str());
}