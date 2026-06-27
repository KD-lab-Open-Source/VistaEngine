// DebugSymbolManager stub for the cross-platform build.
//
// The real one resolves symbol names from a running process via DbgHelp
// (Win32). Off-Windows we report no names; a cross-platform symbolizer is a
// later debugging-tooling concern.
#include <string>
using namespace std;

#include "DebugSymbolManager.h"

DebugSymbolManager* debugSymbolManager = 0;

void DebugSymbolManager::create() {}
DebugSymbolManager::~DebugSymbolManager() {}

bool DebugSymbolManager::getProcName(void* /*proc*/, std::string& /*name*/)
{
	return false;
}
