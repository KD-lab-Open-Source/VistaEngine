// LibraryEditorDialog.h — Qt port of kdw::LibraryEditorDialog
// (Util/kdw/LibraryEditorDialog.h).
//
// A modal dialog wrapping a LibraryEditor, opened from the Libraries menu.
// Qt-clean: the editor reaches the engine through IWorldBridge.

#pragma once

#include <QDialog>

#include <string>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class LibraryEditor;

class LibraryEditorDialog : public QDialog
{
	Q_OBJECT
public:
	explicit LibraryEditorDialog(QWidget* parent = nullptr);

	// The engine bridge the editor talks to (set by MainWindow).
	void setBridge(IWorldBridge* bridge) { bridge_ = bridge; }

	// Open the given library and show the dialog modally.
	void openLibrary(const std::string& libraryName);

private:
	IWorldBridge* bridge_ = nullptr;
	LibraryEditor* editor_ = nullptr;
};