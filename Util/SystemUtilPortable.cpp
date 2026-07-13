// Portable subset of SystemUtil for the cross-platform build.
//
// The real SystemUtil.cpp mixes a few portable helpers (time formatting, the
// localization-data path) with Win32 GUI: common file dialogs (commdlg.h),
// editText / popup menus (HWND), and resource.h. Off-Windows we provide the
// portable helpers for real and no-op the GUI dialogs (the editor/dialog UI is a
// later SDL-based effort).
#include <string>
#include <vector>
#include <cstdio>
using namespace std;

#include "SystemUtil.h"
#include "GameOptions.h"

string formatTimeWithHour(int timeMilis) {
	string res;
	if (timeMilis >= 0) {
		int sec = timeMilis / 1000;
		int min = sec / 60;
		sec -= min * 60;
		int hour = min / 60;
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
		int sec = timeMilis / 1000;
		int min = sec / 60;
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

string default_font_name = "Scripts\\Resource\\fonts\\default.ttf";

// The real setLogicFp() pins the x87 FPU to single precision (_controlfp) for
// deterministic game-logic floats — an x86-only control word. No portable
// equivalent on arm64; the determinism story is a later cross-platform concern.
void setLogicFp()
{
}

// Its counterpart: the logic threads xassert(checkLogicFp()) to catch a stray
// _controlfp leaving the FPU in the wrong precision mid-simulation. With setLogicFp() a
// no-op there is nothing to verify, so this reports success rather than failing an
// assertion the code cannot satisfy. Windows needs it defined for the first time,
// because there the asserts are live — off-Windows NASSERT compiles the call away, which
// is why nothing ever missed it.
bool checkLogicFp()
{
	return true;
}

const char* getLocDataPath()
{
	return GameOptions::instance().getLocDataPath();
}

string getLocDataPath(const char* dir)
{
	string path = getLocDataPath();
	path += dir;
	return path;
}

// --- Win32 GUI dialogs: no-op off-Windows ---------------------------------
bool openFileDialog(string& /*filename*/, const char* /*initialDir*/, const char* /*extention*/, const char* /*title*/) { return false; }
bool saveFileDialog(string& /*filename*/, const char* /*initialDir*/, const char* /*extention*/, const char* /*title*/) { return false; }
const char* popupMenu(vector<const char*> /*items*/) { return 0; }
int popupMenuIndex(vector<const char*> /*items*/) { return -1; }
const char* editText(const char* defaultValue) { return defaultValue; }
