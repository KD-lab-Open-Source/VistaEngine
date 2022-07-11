#ifndef __SYSTEM_UTIL_H__
#define __SYSTEM_UTIL_H__

/////////////////////////////////////////////////////////////////////////////////
//		Memory check
/////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define DBGCHECK
//void win32_check();
//#define DBGCHECK	win32_check();
#else
#define DBGCHECK
#endif

/////////////////////////////////////////////////////////////////////////////////
//		File find
/////////////////////////////////////////////////////////////////////////////////
const char* win32_findfirst(const char* mask);
const char* win32_findnext();


/////////////////////////////////////////////////////////////////////////////////
//		Key Press
/////////////////////////////////////////////////////////////////////////////////
#define VK_TILDE	0xC0
#define VK_LDBL		0x88
#define VK_RDBL		0x89
#define VK_WHEELUP	0xA7
#define VK_WHEELDN	0xA8

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST + 1)
#endif

#define MK_MENU 0x0080

bool applicationHasFocus();

inline bool isPressed(int key) { return applicationHasFocus() && (GetAsyncKeyState(key) & 0x8000); } 
inline bool isShiftPressed() { return isPressed(VK_SHIFT); }
inline bool isControlPressed() { return isPressed(VK_CONTROL); }
inline bool isAltPressed() { return isPressed(VK_MENU); }

const unsigned int KBD_CTRL = 1 << 8;
const unsigned int KBD_SHIFT = 1 << 9;
const unsigned int KBD_MENU = 1 << 10;

// ---   Ini file   ------------------------------
class IniManager
{
	const char* fname_;
	bool check_existence_;
public:
	IniManager(const char* fname, bool check_existence = true) { fname_ = fname; check_existence_ = check_existence; }
	const char* get(const char* section, const char* key);
	void put(const char* section, const char* key, const char* val);
	bool getBool(const char* section, const char* key, bool& value);
	int getInt(const char* section, const char* key);
	bool getInt(const char* section, const char* key, int& value);
	void putInt(const char* section, const char* key, int val);
	float getFloat(const char* section, const char* key);
	bool getFloat(const char* section, const char* key, float& value);
	void putFloat(const char* section, const char* key, float val);
	void getFloatArray(const char* section, const char* key, int size, float array[]);
	void putFloatArray(const char* section, const char* key, int size, const float array[]);
};

// ---  Files ------------------------------
string setExtention(const char* file_name, const char* extention);
string getExtention(const char* file_name);
string cutPathToResource(const char* nameIn);

bool createDirectory(const char* name);

// --- LocData ------
const char* getLocDataPath();
const char* getLocDataPath(const char* dir);
const char* setLocDataPath(const char* fullName);

// --- Registry ------
string getStringFromReg(const string& folderName, const string& keyName);
void putStringToReg(const string& folderName, const string& keyName, const string& value);

// --- Formatting ------
string formatTimeWithHour(int timeMilis);
string formatTimeWithoutHour(int timeMilis);

//-------------------------------------------------
bool openFileDialog(string& filename, const char* initialDir, const char* extention, const char* title);
bool saveFileDialog(string& filename, const char* initialDir, const char* extention, const char* title);
const char* popupMenu(vector<const char*> items); // returns zero if cancel
int popupMenuIndex(vector<const char*> items); // returns -1 if cancel
const char* editText(const char* defaultValue);
const char* editTextMultiLine(const char* defaultValue, HWND hwnd);

//-------------------------------------------------
void setLogicFp();
bool checkLogicFp();

//-------------------------------------------------
string transliterate(const char* name);

extern string default_font_name;

#endif //__SYSTEM_UTIL_H__
