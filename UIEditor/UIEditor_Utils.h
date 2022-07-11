#ifndef __UIEDITOR_UTILS_H_INCLUDED__
#define __UIEDITOR_UTILS_H_INCLUDED__

#include "XTL/Rect.h"

typedef std::vector<Rectf*> SubRects;

class CUIMainFrame;
CUIMainFrame* uiEditorFrame();
void saveAllLibraries();
void saveInterfaceLibraries();

const char* check_command_line(const char* switch_str);

#endif
