// Console-window stub for the cross-platform build.
//
// The real ConsoleWindow is a Win32 HWND debug console. Off-Windows it is inert
// (never visible); a cross-platform debug console is a later concern.
#include <string>
#include <vector>
using namespace std;

#include "xutil.h"   // XStream, used by Console.h
#include "ConsoleWindow.h"

ConsoleWindow::ConsoleWindow() {}
ConsoleWindow::~ConsoleWindow() {}
void ConsoleWindow::show(bool) {}
bool ConsoleWindow::isVisible() const { return false; }

// ConsoleListener overrides (defining them emits ConsoleWindow's vtable).
void ConsoleWindow::init(Console* /*console*/) {}
void ConsoleWindow::detach() {}
void ConsoleWindow::writeMessage(const Console::Message& /*msg*/) {}
