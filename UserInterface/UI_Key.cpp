#include "StdAfx.h"
#include "Serialization\Serialization.h"
#include "UserInterface\UI_Key.h"
#include "WBuffer.h"

UI_Key::PairComboStrings UI_Key::locStringsKeyNames_;

const char* toHex(unsigned char byte);

bool UI_Key::useLocalizedNames_;
UI_Key::LocStrings UI_Key::locNames_;
LocString UI_Key::nameCtrl_;
LocString UI_Key::nameShift_;
LocString UI_Key::nameMenu_;

struct Initer{
	Initer() { UI_Key::initLocale(); }
} doIt;


void UI_Key::initLocale()
{
	useLocalizedNames_ = false;
	locNames_.resize(256);
	locStringsKeyNames_.clear();

	XBuffer nameMain(16), nameAlt(32);
	for(int idx = 0; idx < 256; ++idx){
		nameMain.init();
		nameMain < "VK_KEY_" < toHex(idx);

		nameAlt.init();
		nameAlt < "VK_KEY_" < toHex(idx);
		if(*vk_table_[idx])
			nameAlt < " (" < vk_table_[idx] < ")";

		locStringsKeyNames_.push_back(make_pair(nameMain.c_str(), nameAlt.c_str()));
	};
	xassert(locNames_.size() == locStringsKeyNames_.size());
}

bool UI_Key::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	static const char* typeName = typeid(sKey).name();
	if(ar.isEdit())
		return __super::serialize(ar, name, nameAlt);
	else 
		return ar.serialize(fullkey, name, nameAlt);
}

void UI_Key::serializeLocale(Archive& ar)
{
	xxassert(locNames_.size() == locStringsKeyNames_.size(), "не инициализирована локализация sKey");
	ar.serialize(useLocalizedNames_, "useLocalizedNames", "&Использовать локализованные клавиши");
	if(useLocalizedNames_ || !ar.isEdit()){
		if(ar.openBlock("loc names", "локализация клавиш")){
			ar.serialize(nameCtrl_, "nameCtrl", "Control");
			ar.serialize(nameShift_, "nameShift", "Shift");
			ar.serialize(nameMenu_, "nameMenu", "Alt");
			int idx = 0;
			LocStrings::iterator it = locNames_.begin();
			for(; it != locNames_.end(); ++it, ++idx)
				ar.serialize(*it, locStringsKeyNames_[idx].first.c_str(), locStringsKeyNames_[idx].second.c_str());
			ar.closeBlock();
		}
	}
}


const wchar_t* UI_Key::toString(WBuffer& keyName) const
{
	keyName.init();
	
	if(ctrl && key != VK_CONTROL)
		keyName < UI_Key::nameCtrl();
	
	if(shift && key != VK_SHIFT){
		if(keyName.tell())
			keyName < L" + ";
		keyName < UI_Key::nameShift();
	}
	
	if(menu && key != VK_MENU){
		if(keyName.tell())
			keyName < L" + ";
		keyName < UI_Key::nameMenu(); // Alt
	}

	if(key){
		if(keyName.tell())
			keyName < L" + ";

		const wchar_t* locName = UI_Key::name(key);
		if(locName && *locName)
			keyName < locName;
		else{
			keyName < L"0x";
			keyName < toHex(key);
		}
	}
	return keyName.c_str();
}
