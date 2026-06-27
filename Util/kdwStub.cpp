// kdw (kdWidgets editor toolkit) stub for the cross-platform build.
//
// kdw is a Win32-GDI widget toolkit used by the in-game editor/property dialogs.
// It is Windows-only (gated: add_subdirectory(kdw) only on WIN32). Off-Windows
// the editor UI is unavailable, so the property-editor entry points and the
// global hooks are no-ops; a cross-platform editor is a separate effort.
#include <string>
#include <vector>
using namespace std;

// Before the kdw headers: Widget.h pulls XTL/Rect.h whose serialize() needs a
// complete Archive (incomplete otherwise under clang).
#include "Serialization/Serialization.h"
#include "kdw/PropertyEditor.h"
#include "kdw/ContentUtil.h"
#include "kdw/Win32/Window.h"
#include "kdw/Dialog.h"
#include "kdw/Entry.h"
#include "kdw/Label.h"
#include "kdw/LibraryEditorDialog.h"
#include "kdw/HotkeyContext.h"

namespace kdw {

GetAllTextureNamesFunc getAllTextureNamesFunc = 0;

// --- editor dialog widgets (the real impls are the gated Win32 kdw library) ---
Dialog::Dialog(HWND /*owner*/, int border) : Window(border) {}
Dialog::~Dialog() {}
int  Dialog::showModal() { return 0; }
void Dialog::add(Widget* /*widget*/, bool, bool, bool) {}
void Dialog::addButton(const char* /*label*/, int /*response*/, bool /*atRight*/) {}

Entry::Entry(const char* /*text*/, bool /*multline*/, int border) : _WidgetWithWindow(0, border) {}
Entry::~Entry() {}
void Entry::serialize(Archive& /*ar*/) {}

Label::Label(const char* /*text*/, bool /*emphasis*/, int border) : _WidgetWithWindow(0, border) {}
void Label::setAlignment(TextAlignHorizontal, TextAlignVertical) {}
void Label::serialize(Archive& /*ar*/) {}

void Window::setTitle(std::string /*str*/) {}

LibraryEditorDialog::LibraryEditorDialog(HWND owner) : Dialog(owner) {}
int  LibraryEditorDialog::showModal(const char* /*libraryName*/) { return 0; }
void LibraryEditorDialog::serialize(Archive& /*ar*/) {}

// --- base widget hierarchy (Widget -> Container -> Window -> Dialog, and
// _WidgetWithWindow) the dialogs drag in; no-op so the vtables resolve ---
void Widget::show() {}
void Widget::hide() {}
void Widget::setRequestSize(const Vect2i /*size*/) {}
void Widget::passFocus(FocusDirection /*direction*/) {}
void Widget::_setParent(Container* /*container*/) {}
void Widget::_setPosition(const Recti& /*position*/) {}
void Widget::_queueRelayout() {}
void Widget::_setMinimalSize(const Vect2i& /*size*/) {}
void Widget::_setVisibleInLayout(bool /*visibleInLayout*/) {}

// isVisible() overrides the pure Widget::isVisible() and, being the first
// out-of-line virtual, is each concrete class's vtable key function.
bool Container::isVisible() const { return false; }
bool Container::isActive() const { return false; }
void Container::setBorder(int /*border*/) {}
void Container::_setFocus() {}
Widget* Container::_nextWidget(Widget* /*last*/, FocusDirection /*direction*/) { return 0; }
bool Container::_focusable() const { return false; }

Window::Window(int /*border*/, int /*style*/) {}
Window::~Window() {}
bool Window::isVisible() const { return false; }
void Window::showAll() {}
void Window::visitChildren(WidgetVisitor& /*visitor*/) const {}
void Window::_updateVisibility() {}
void Window::_arrangeChildren() {}
void Window::_relayoutParents() {}
void Window::_setFocus() {}

void Dialog::onResponse(int /*response*/) {}
void Dialog::onClose() {}
void Dialog::serialize(Archive& /*ar*/) {}
void Dialog::onKeyDefault() {}
void Dialog::onKeyCancel() {}

_WidgetWithWindow::_WidgetWithWindow(_WidgetWindow* /*window*/, int /*border*/) {}
_WidgetWithWindow::~_WidgetWithWindow() {}
void _WidgetWithWindow::setSensitive(bool /*sensitive*/) {}
bool _WidgetWithWindow::isVisible() const { return false; }
void _WidgetWithWindow::_setPosition(const Recti& /*position*/) {}
void _WidgetWithWindow::_setParent(Container* /*container*/) {}
void _WidgetWithWindow::_setFocus() {}
void _WidgetWithWindow::_updateVisibility() {}

Widget::Widget() {}
Widget::~Widget() {}
void Widget::showAll() {}
void Widget::setBorder(int /*border*/) {}
void Widget::serialize(Archive& /*ar*/) {}
void Widget::_relayoutParents() {}
Widget* Widget::_nextWidget(Widget* /*last*/, FocusDirection /*direction*/) { return 0; }
void Widget::_setFocus() {}
void Widget::_updateVisibility() {}

void Window::onClose() {}
void Window::serialize(Archive& /*ar*/) {}

HotkeyContext::~HotkeyContext() {}

bool edit(Serializer& /*ser*/, const char* /*stateFileName*/, int /*flags*/,
		  Widget* /*parent*/, const char* /*title*/)
{
	return false;
}

bool edit(Serializer& /*ser*/, const char* /*stateFileName*/, int /*flags*/,
		  HWND /*parent*/, const char* /*title*/)
{
	return false;
}

} // namespace kdw

namespace Win32 {

void _setGlobalInstance(HINSTANCE /*instance*/) {}

} // namespace Win32
