#include "StdAfx.h"
#include "SystemUtil.h"
#include "RenderObjects.h"
#include "GameOptions.h"
#include <crtdbg.h>
#include <commdlg.h>
#include "..\resource.h"
#include "float.h"

string default_font_name="Scripts\\Resource\\fonts\\Arial.font";

/////////////////////////////////////////////////////////////////////////////////
//		Memory check
/////////////////////////////////////////////////////////////////////////////////
void win32_check()
{
	_ASSERTE(_CrtCheckMemory()) ;
}

/////////////////////////////////////////////////////////////////////////////////
//		File find
/////////////////////////////////////////////////////////////////////////////////
static WIN32_FIND_DATA FFdata;
static HANDLE FFh;

const char* win32_findnext()
{
	if(FindNextFile(FFh,&FFdata) == TRUE){
		//if(FFdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return win32_findnext();
		return FFdata.cFileName;
		}
	else {
		FindClose(FFh);
		return NULL;
		}
}

const char* win32_findfirst(const char* mask)
{
	FFh = FindFirstFile(mask,&FFdata);
	if(FFh == INVALID_HANDLE_VALUE) return NULL;
	//if(FFdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return win32_findnext();
	return FFdata.cFileName;
}

// ---   Ini file   ---------------------
const char* IniManager::get(const char* section, const char* key)
{
	static char buf[256];
	static char path[_MAX_PATH];

	if(_fullpath(path,fname_,_MAX_PATH) == NULL) 
		ErrH.Abort("Ini file not found: ", XERR_USER, 0, fname_);
	if(!GetPrivateProfileString(section,key,NULL,buf,256,path)){
		*buf = 0;
	}

	return buf;
}
void IniManager::put(const char* section, const char* key, const char* val)
{
	static char path[_MAX_PATH];

	if(_fullpath(path,fname_,_MAX_PATH) == NULL) {
		ErrH.Abort("Ini file not found: ", XERR_USER, 0, fname_);
	}

	WritePrivateProfileString(section,key,val,path);
}

int IniManager::getInt(const char* section, const char* key) 
{ 
	return atoi(get(section, key));
}

bool IniManager::getBool(const char* section, const char* key, bool& value) 
{
	const char* str=get(section, key);
	if(*str) {
		value=bool(atoi(str));
		return true;
	}
	else return false;

}

bool IniManager::getFloat(const char* section, const char* key, float& value) 
{
	const char* str=get(section, key);
	if(*str) {
		value=atof(str);
		return true;
	}
	else return false;

}

bool IniManager::getInt(const char* section, const char* key, int& value) 
{
	const char* str=get(section, key);
	if(*str) {
		value=atoi(str);
		return true;
	}
	else return false;

}
void IniManager::putInt(const char* section, const char* key, int val) 
{
	char buf [256];
	put(section, key, itoa(val, buf, 10));
}

float IniManager::getFloat(const char* section, const char* key) 
{ 
	return atof(get(section, key)); 
}
void IniManager::putFloat(const char* section, const char* key, float val) 
{
	XBuffer buf;
	buf <= val;
	put(section, key, buf);
}

void IniManager::getFloatArray(const char* section, const char* key, int size, float array[])
{
	const char* str = get(section, key); 
	XBuffer buf((void*)str, strlen(str) + 1);
	for(int i = 0; i < size; i++)
		buf >= array[i];
}
void IniManager::putFloatArray(const char* section, const char* key, int size, const float array[])
{
	XBuffer buf(256, 1);
	for(int i = 0; i < size; i++)
		buf <= array[i] < " ";
	put(section, key, buf);
}

string getStringFromReg(const string& folderName, const string& keyName) {
	string res;
	HKEY hKey;
	const int maxLen = 64;
	char name[maxLen];
	DWORD nameLen = maxLen;
	LONG lRet;

	lRet = RegOpenKeyEx( HKEY_CURRENT_USER, folderName.c_str(), 0, KEY_QUERY_VALUE, &hKey );

	if ( lRet == ERROR_SUCCESS ) {
		lRet = RegQueryValueEx( hKey, keyName.c_str(), NULL, NULL, (LPBYTE) name, &nameLen );

		if ( (lRet == ERROR_SUCCESS) && nameLen && (nameLen <= maxLen) ) {
			res = name;
		}

		RegCloseKey( hKey );
	}
	return res;
}
void putStringToReg(const string& folderName, const string& keyName, const string& value) {
	HKEY hKey;
	DWORD dwDisposition;
	LONG lRet;
	
	lRet = RegCreateKeyEx( HKEY_CURRENT_USER, folderName.c_str(), 0, "", 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition );

	if ( lRet == ERROR_SUCCESS ) {
		lRet = RegSetValueEx( hKey, keyName.c_str(), 0, REG_SZ, (LPBYTE) (value.c_str()), value.length() );

		RegCloseKey( hKey );
	}
}

string formatTimeWithHour(int timeMilis) {
	string res;
	if (timeMilis >= 0) {
		int sec = timeMilis / 1000.0f;
		int min = sec / 60.0f;
		sec -= min * 60;
		int hour = min / 60.0f; 
		min -= hour * 60; 
		char str[11];
		sprintf(str, "%d", hour);
		res = (hour < 10) ? "0" : "";
		res += string(str) + ":";
		sprintf(str, "%d", min);
		res += (min < 10) ? "0" : "";
		res += string(str) + ":";
		sprintf(str, "%d", sec);
		res += (sec < 10) ? "0" : "";
		res += string(str);
	}
	return res;
}

string formatTimeWithoutHour(int timeMilis) {
	string res;
	if (timeMilis >= 0) {
		int sec = timeMilis / 1000.0f;
		int min = sec / 60.0f;
		sec -= min * 60;
		char str[11];
		sprintf(str, "%d", min);
		res = (min < 10) ? "0" : "";
		res += string(str) + ":";
		sprintf(str, "%d", sec);
		res += (sec < 10) ? "0" : "";
		res += string(str);
	}
	return res;
}

//-------------------------------------------------
bool openFileDialog(string& filename, const char* initialDir, const char* extention, const char* title)
{
	XBuffer filter;
	filter < title < '\0' < "*." < extention < '\0' < '\0';

	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	char fname[2048];
	strcpy(fname,filename.c_str());
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gb_RenderDevice->GetWindowHandle();
	string fullTitle = string("Open: ") + title;
	ofn.lpstrTitle = fullTitle.c_str();
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = sizeof(fname)-1;
	ofn.lpstrInitialDir = initialDir;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = extention;
	if(!GetOpenFileName(&ofn))
		return false;
	filename = fname;
	return true;
}

bool saveFileDialog(string& filename, const char* initialDir, const char* extention, const char* title)
{
	XBuffer filter;
	filter < title < '\0' < "*." < extention < '\0' < '\0';

	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	char fname[2048];
	strcpy(fname,filename.c_str());
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gb_RenderDevice->GetWindowHandle();
	string fullTitle = string("Save: ") + title;
	ofn.lpstrTitle = fullTitle.c_str();
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = sizeof(fname)-1;
	ofn.lpstrInitialDir = initialDir;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_EXPLORER|OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = extention;
	if(!GetSaveFileName(&ofn))
		return false;
	filename = fname;
	return true;
}

const char* popupMenu(vector<const char*> items) // returns zero if cancel
{
	if(items.empty())
		return 0;

	HMENU hMenu = CreatePopupMenu();
	
	vector<const char*>::iterator i;
	FOR_EACH(items, i)
		AppendMenu(hMenu, MF_STRING, 1 + i - items.begin(), *i);
	
	POINT point; 
	GetCursorPos(&point);
	int index = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, gb_RenderDevice->GetWindowHandle(), 0);
	
	DestroyMenu(hMenu);
	
	if(index > 0 && index <= items.size())
		return items[index - 1];
	else
		return 0;
}

int popupMenuIndex(vector<const char*> items) // returns -1 if cancel
{
	if(items.empty())
		return -1;

	HMENU hMenu = CreatePopupMenu();
	
	vector<const char*>::iterator i;
	FOR_EACH(items, i)
		AppendMenu(hMenu, MF_STRING, 1 + i - items.begin(), *i);
	
	POINT point; 
	GetCursorPos(&point);
	int index = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, gb_RenderDevice->GetWindowHandle(), 0);
	
	DestroyMenu(hMenu);
	
	if(index > 0 && index <= items.size())
		return index - 1;
	else
		return -1;
}



//-----------------------------------------
static string editTextString;
static BOOL CALLBACK DialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_SYSCOMMAND:
		if(wParam==SC_CLOSE)
		{
			EndDialog(hwnd,IDCANCEL);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			char tmpstr[256];
			HWND h;

			h=GetDlgItem(hwnd,IDC_INPUT_TEXT);
			GetWindowText(h,tmpstr,256);
			editTextString = tmpstr;

			EndDialog(hwnd,IDOK);
			break;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hwnd,IDCANCEL);
			break;
		}
		break;
	case WM_INITDIALOG:
		{
			HWND h;
			h=GetDlgItem(hwnd,IDC_INPUT_TEXT);
			SetWindowText(h, editTextString.c_str());
		}
		return TRUE;
	}
	return FALSE;
}

const char* editText(const char* defaultValue)
{
	editTextString = defaultValue;
	int ret = DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(IDD_DIALOG_INPUT_TEXT),gb_RenderDevice->GetWindowHandle(),DialogProc);
//	if(ret!=IDOK)
//		return 0;
	return editTextString.c_str();
}

const char* editTextMultiLine(const char* defaultValue, HWND hwnd)
{
	editTextString = defaultValue;
	int ret = DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(IDD_DIALOG_INPUT_TEXT_MULTILINE),hwnd,DialogProc);
//	if(ret!=IDOK)
//		return 0;
	return editTextString.c_str();
}

/////////////////////////////////////////
#define GAME_LOGIC_FLOAT_POINT_PRECISION _PC_24
void setLogicFp()
{
	_controlfp(GAME_LOGIC_FLOAT_POINT_PRECISION, _MCW_PC); //_PC_24

#ifndef _FINAL_VERSION_
	//static int enable = false;
	//IniManager("Game.ini").getInt("Game", "ControlFpEnable", enable);
	static int enable = IniManager("Game.ini").getInt("Game", "ControlFpEnable", enable) ? enable : false;
	if(enable){
		_controlfp( _controlfp(0,0) & ~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL |  EM_INVALID),  MCW_EM ); 
		_clearfp();
	}
#endif
}
bool checkLogicFp()
{
	return (_controlfp(0,0)&_MCW_PC) == GAME_LOGIC_FLOAT_POINT_PRECISION;
}

//////////////////////////////////////////
bool createDirectory(const char* name)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(name, &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE)
		return CreateDirectory(name, NULL);
	else{
		FindClose(hFind);
		return false;
	}
}

string setExtention(const char* file_name, const char* extention)
{
	string str = file_name;
	unsigned int pos = str.rfind(".");
	if(pos != string::npos)
		str.erase(pos, str.size());
	if(!*extention)
		return str;
	return str + "." + extention;
}

string getExtention(const char* file_name)
{
	string str = file_name;
	unsigned int pos = str.rfind(".");
	if(pos != string::npos){
		str.erase(0, pos + 1);
		if(str.empty())
			return "";
		strlwr((char*)str.c_str());
		while(isspace(str[str.size() - 1]))
			str.erase(str.size() - 1);
		return str;
	}
	else
		return "";
}

string cutPathToResource(const char* nameIn)
{
	string name = nameIn;
	strlwr((char*)name.c_str());
	size_t pos = name.rfind("resource\\");
	if(pos != string::npos)
		name.erase(0, pos);
	return name;
}


const char* getLocDataPath() 
{
	return GameOptions::instance().getLocDataPath();
}

const char* getLocDataPath(const char* dir) 
{
	static string path;
	path = string(getLocDataPath()) + dir;
	return path.c_str();
}

const char* setLocDataPath(const char* fullName)
{
	// "Resource\\LocData\\English\\Sounds\\sound.wav"
	static string name;
	name = fullName;
	int pos = name.find("\\");
	if(pos != string::npos){
		pos = name.find("\\", pos + 1);
		if(pos != string::npos){
			pos = name.find("\\", pos + 1);
			name.erase(0, pos + 1);
		}
	}
	name = getLocDataPath() + name;
	return name.c_str();
}

string transliterate(const char* name)
{
	// Транслитерация неточная с точки зрения чтения, но удовлетворяющая требованиям
	// имен переменных - не должно быть цифр (Ч - 4) и знаков (Ъ - ')
	static const char* table[256] = {
		"\x0", "\x1", "\x2", "\x3", "\x4", "\x5", "\x6", "\x7", "\x8", "\x9", "\xa", "\xb", 
		"\xc", "\xd", "\xe", "\xf", "\x10", "\x11", "\x12", "\x13", "\x14", "\x15", "\x16", 
		"\x17", "\x18", "\x19", "\x1a", "\x1b", "\x1c", "\x1d", "\x1e", "\x1f", "_", 
		"\x21", "\x22", "\x23", "\x24", "\x25", "\x26", "_", "_", "_", "_", 
		"_", "_", "_", "_", "_", 
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@", 
		"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", 
		"R", "S", "T", "U", "V", "W", "X", "Y", "Z", "_", "_", "_", "_", "_", "_", "a", "b", 
		"c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", 
		"t", "u", "v", "w", "x", "y", "z", "_", "_", "_", "_", "", 
		
/*		DOS
		//"А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", 
		"A", "B", "V", "G", "D", "E", "J", "Z", "I", "J", "K", "L", "M", "N", "O", "P", "R", 
		//"С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я", 
		"S", "T", "U", "F", "X", "C", "Ch", "Sh", "Sh", "h", "I", "h", "E", "U", "Ja", 
		//"а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", 
		"a", "b", "v", "g", "d", "e", "j", "z", "i", "j", "k", "l", "m", "n", "o", "p", 
		"-", "-", "-", "¦", "+", "¦", "¦", "¬", "¬", "¦", "¦", "¬", "-", "-", "-", "¬", 
		"L", "+", "T", "+", "-", "+", "¦", "¦", "L", "г", "¦", "T", "¦", "=", "+", "¦", 
		"¦", "T", "T", "L", "L", "-", "г", "+", "+", "-", "-", "-", "-", "¦", "¦", "-", 
		//"р", "с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я", 
		"r", "s", "t", "u", "f", "x", "c", "ch", "sh", "sh", "h", "i", "h", "e", "u", "ja", 
		//"Ё", "ё", "Є", "є", "Ї", "ї", "Ў", "ў", "°", "•", "·", "v", "№", "¤", "¦", " " 
		"E", "e", "Є", "є", "Ї", "ї", "Ў", "ў", "°", "•", "·", "v", "№", "¤", "¦", " " 
*/
		"_", "_", "'", "_", "\"", ":", "+", "+", "_", "%", "_", "<", "_", "_", "_", "_", "_", 
		"'", "'", "\"", "\"", "", "-", "-", "_", "T", "_", ">", "_", "_", "_", "_", " ", "Ў", 
		"ў", "_", "¤", "_", "¦", "", "E", "c", "Є", "<", "¬", "-", "R", "Ї", "°", "+", "_", 
		"_", "_", "ч", "", "·", "e", "№", "є", ">", "_", "_", "_", "ї", 
		//"А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", 
		"A", "B", "V", "G", "D", "E", "J", "Z", "I", "J", "K", "L", "M", "N", "O", "P", "R", 
		//"С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я", 
		"S", "T", "U", "F", "X", "C", "Ch", "Sh", "Sh", "b", "I", "b", "E", "U", "Ja", 
		//"а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", 
		"a", "b", "v", "g", "d", "e", "j", "z", "i", "j", "k", "l", "m", "n", "o", "p", 
		//"с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я"
		"r", "s", "t", "u", "f", "x", "c", "ch", "sh", "sh", "b", "i", "b", "e", "u", "ja"
	};
	
	string result;
	while(*name){
		int c = unsigned char(*name++);
		result += table[c];
	}
	return result;
}

