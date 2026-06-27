// TriggerEditor stub for the cross-platform build.
//
// TriggerEditor is a kdw (Win32) editor dialog for trigger chains. The kdw
// toolkit is Windows-only, so off-Windows the editor is unavailable: the dialog
// constructs but edit() is a no-op. A cross-platform trigger editor is a later
// effort. (TriggerExport.cpp — the non-UI serialization — is built for real.)
#include <string>
#include <vector>
using namespace std;

#include "Serialization/Serialization.h"
#include "TriggerEditor.h"

TriggerEditor::TriggerEditor(TriggerChain& triggerChain, HWND hwnd, bool /*toCopy*/)
	: kdw::Dialog(hwnd), triggerChain_(triggerChain),
	  commandManager_(0), vSplitter1_(0), hSplitter_(0), vSplitter2_(0),
	  triggerView_(0), miniMap_(0), debugger_(0), actionsTree_(0)
{}

bool TriggerEditor::edit() { return false; }
