#ifndef __SKEY_H__
#define __SKEY_H__

#include "SystemUtil.h"
#include "LocString.h"

class Archive;

struct sKey
{
	union
	{
		struct
		{	
			unsigned char key;
			unsigned char ctrl : 1;
			unsigned char shift : 1;
			unsigned char menu	: 1;
		};
		int fullkey;
	};

	sKey(int fullkey_ = 0, bool set_by_async_funcs = false) { 
		fullkey = fullkey_;
		if(set_by_async_funcs){ 
			ctrl = isControlPressed(); 
			shift = isShiftPressed(); 
			menu = isAltPressed(); 
		} 
		// добавл€ем расширенные коды дл€ командных кодов
		if(key == VK_CONTROL) 
			ctrl |= 1;
		if(key == VK_SHIFT)
			shift |= 1;
		if(key == VK_MENU)
			menu |= 1;
	}
	bool pressed() const {
		return isPressed(key) && !(ctrl ^ isControlPressed()) && !(shift ^ isShiftPressed()) && !(menu ^ isAltPressed());
	}
	bool keyPressed() const { // int mask = KBD_CTRL | KBD_SHIFT | KBD_MENU
		return isPressed(key);
	}
	bool keyPressed(int mask) const {
		return isPressed(key) 
			&& ((mask & KBD_CTRL)	? true : !(ctrl ^ isControlPressed()))
			&& ((mask & KBD_SHIFT)	? true : !(shift ^ isShiftPressed()))
			&& ((mask & KBD_MENU)	? true : !(menu ^ isAltPressed()));
	}
	bool operator == (int _fullKey) const { return fullkey == _fullKey; }
	bool operator == (const sKey& key) const { return fullkey == key.fullkey; }
	bool operator != (const sKey& key) const { return fullkey != key.fullkey; }

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	static void initLocale();
	static void serializeLocale(Archive& ar);

	static const char* name(int fullKey) { return useLocalizedNames_ ? locNames_[fullKey & 0xFF].c_str() : vk_table_[fullKey & 0xFF]; }
	static const char* nameCtrl() { return useLocalizedNames_ ? nameCtrl_.c_str() : "Ctrl"; }
	static const char* nameShift() { return useLocalizedNames_ ? nameShift_.c_str() : "Shift"; }
	static const char* nameMenu() { return useLocalizedNames_ ? nameMenu_.c_str() : "Alt"; }

	string toString() const;

private:
	static const char* vk_table_[];

	typedef vector<std::pair<string, string> > PairComboStrings;
	static PairComboStrings locStringsKeyNames_;

	static bool useLocalizedNames_;

	typedef vector<LocString> LocStrings;
	static LocStrings locNames_;
	static LocString nameCtrl_;
	static LocString nameShift_;
	static LocString nameMenu_;
};

#endif //__SKEY_H__
