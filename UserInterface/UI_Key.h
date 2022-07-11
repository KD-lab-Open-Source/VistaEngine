#ifndef __UI_KEY_H_INCLUDED__
#define __UI_KEY_H_INCLUDED__

#include "SystemUtil.h"
#include "sKey.h"
#include "LocString.h"

struct UI_Key : sKey{
	explicit UI_Key(int fullKey = 0) : sKey(fullKey) {}
	UI_Key(sKey key) : sKey(key) {}
	
	bool keyPressed(int mask) const {
		return isPressed(key) 
			&& ((mask & KBD_CTRL)	? true : ctrl == isControlPressed())
			&& ((mask & KBD_SHIFT)	? true : shift == isShiftPressed())
			&& ((mask & KBD_MENU)	? true : menu == isAltPressed());
	}
	bool pressed() const { return keyPressed(0); }
	bool keyPressed() const { return keyPressed(KBD_CTRL | KBD_SHIFT | KBD_MENU); }

	bool check(int fkey, int mask = KBD_SHIFT) const { return fullkey && (fullkey & ~mask) == (fkey & ~mask); }
	bool check(const sKey& key, int mask = KBD_SHIFT) const { return fullkey && (fullkey & ~mask) == (key.fullkey & ~mask); }

	static void initLocale();
	static void serializeLocale(Archive& ar);

	const wchar_t* toString(class WBuffer& keyName) const;

	bool serialize(Archive& ar, const char* name, const char* nameAlt);
protected:

	static const wchar_t* name(int fullKey) { return locNames_[fullKey & 0xFF].c_str(); }

	static const wchar_t* nameCtrl()  { return useLocalizedNames_ ? nameCtrl_.c_str()  : L"Ctrl";  }
	static const wchar_t* nameShift() { return useLocalizedNames_ ? nameShift_.c_str() : L"Shift"; }
	static const wchar_t* nameMenu()  { return useLocalizedNames_ ? nameMenu_.c_str()  : L"Alt";  }

	typedef vector<std::pair<string, string> > PairComboStrings;
	static PairComboStrings locStringsKeyNames_;

	static bool useLocalizedNames_;

	typedef vector<LocString> LocStrings;
	static LocStrings locNames_;
	static LocString nameCtrl_;
	static LocString nameShift_;
	static LocString nameMenu_;
};

#endif
