#include "StdAfx.h"
#include "sKey.h"

#include "Serialization.h"
#include "TreeEditor.h"

#include "AttribEditorInterface.h"
AttribEditorInterface& attribEditorInterface();

const char* toHex(unsigned char byte)
{
	static char char_values[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	static char byteBuf[3];
	byteBuf[0] = char_values[byte >> 4];
	byteBuf[1] = char_values[byte & 0x0F];
	byteBuf[2] = '\0';
	return byteBuf;
}

const char* sKey::vk_table_[]  = {
	"", "LB", "RB", "Cancel", "MB", "", "", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "", //0x00 - 0x0F
		"Shift", "Control", "Alt", "Pause", "Caps", "", "", "", "", "", "", "Escape", "", "", "", "", //0x10 - 0x1F
		"Space", "PgUP", "PgDN", "End", "Home", "Left", "Up", "Right", "Down", "", "", "", "PrSrc", "Ins", "Del", "", //0x20 - 0x2F
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "", "", "", "", "", //0x30 - 0x3F
		"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", //0x40 - 0x4F
		"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "LWin", "RWin", "Apps", "", "Sleep", //0x50 - 0x5F
		"num0", "num1", "num2", "num3", "num4", "num5", "num6", "num7", "num8", "num9", "num*", "num+", "", "num-", "num.", "num/", //0x60 - 0x6F
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", //0x70 - 0x7F
		"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "LDBL", "RDBL", "", "", "", "", "", "", //0x80 - 0x8F
		"NumLock", "ScrollLock", "LShift", "RShift", "LCtrl", "RCtrl", "LAlt", "RAlt", "", "", "", "", "", "", "", "", //0x90 - 0x9F
		"", "", "", "", "", "", "", "WheelUp", "WheelDn", "", "", "", "", "", "", "", //0xA0 - 0xAF
		"", "", "", "", "", "", "", "", "", "", ";", "+", ",", "-", ".", "/", //0xB0 - 0xBF
		"~", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", //0xC0 - 0xCF
		"", "", "", "", "", "", "", "", "", "", "", "[", "\\", "]", "\"", "", //0xD0 - 0xDF
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", //0xE0 - 0xEF
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "WakeUp" //0xF0 - 0xFF
};

sKey::PairComboStrings sKey::locStringsKeyNames_;

bool sKey::useLocalizedNames_;
sKey::LocStrings sKey::locNames_;
LocString sKey::nameCtrl_;
LocString sKey::nameShift_;
LocString sKey::nameMenu_;

struct Initer{
	Initer() { sKey::initLocale(); }
} doIt;

void sKey::initLocale()
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

void sKey::serializeLocale(Archive& ar)
{
	xxassert(locNames_.size() == locStringsKeyNames_.size(), "не инициализирована локализаци€ sKey");
	ar.serialize(useLocalizedNames_, "useLocalizedNames", "&»спользовать локализованные клавиши");
	if(useLocalizedNames_ || !ar.isEdit()){
		ar.openBlock("loc names", "локализаци€ клавиш");
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

bool sKey::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	static const char* typeName = typeid(sKey).name();
	static bool editorRegistered = (attribEditorInterface().isTreeEditorRegistered(typeName) != 0);
	if(ar.isEdit()){
		bool nodeExists = ar.openStruct(name, nameAlt, typeName);
		if(editorRegistered){
			ar.serialize(fullkey, "fullKey", 0);
		}
		else{			
			bool tmp = shift;
			ar.serialize(tmp, "Shift", "&Shift");
			shift = tmp;
			tmp = ctrl;
			ar.serialize(tmp, "Control", "&Control");
			ctrl = tmp;
			tmp = menu;
			ar.serialize(tmp, "Alt", "&Alt");
			menu = tmp;

			string str;
			for(int i = 0x00; i != 0xFF; ++i){
				const char* keyName = sKey::name(i);
				if(keyName && *keyName){
					str += "|";
					str += keyName;
				}
			}
			ComboListString combo(str.c_str(), sKey::name(key));
			ar.serialize(combo, "Key", "& лавиша");

			key = 0;
			if(!combo.value().empty())
				for(int i = 0x00; i != 0xFF; ++i)
					if(combo.value() == sKey::name(i)){
						key = i;
						break;
					}

		}
		ar.closeStruct(name);
		return nodeExists;
	}
	else 
		return ar.serialize(fullkey, name, nameAlt);
}

string sKey::toString() const
{
	string keyName;
	
	if(ctrl && key != VK_CONTROL)
		keyName += sKey::nameCtrl();
	
	if(shift && key != VK_SHIFT){
		if(!keyName.empty())
			keyName += " + ";
		keyName += sKey::nameShift();
	}
	
	if(menu && key != VK_MENU){
		if(!keyName.empty())
			keyName += " + ";
		keyName += sKey::nameMenu(); // Alt
	}

	if(key){
		if(!keyName.empty())
			keyName += " + ";

		const char* locName = sKey::name(key);
		if(locName && *locName)
			keyName += locName;
		else{
			keyName += "0x";
			keyName += toHex(key);
		}
	}
	return keyName;
}
