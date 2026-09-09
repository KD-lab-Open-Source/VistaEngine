// LibraryEditor.h — Qt port of kdw::LibraryEditor (Util/kdw/LibraryEditor.h).
//
// The composite editor for one library: a LibraryTree (elements) on the left
// and a PropertyTree (the selected element's serialized fields) on the right,
// with a Save button that writes the tree back through the bridge.
//
// Qt-clean: talks to the engine only through IWorldBridge.

#pragma once

#include <QWidget>

#include <string>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class LibraryTree;
class PropertyTree;
class QPushButton;

class LibraryEditor : public QWidget
{
	Q_OBJECT
public:
	explicit LibraryEditor(QWidget* parent = nullptr);

	// Open a library: populate the element tree and show the first element.
	void openLibrary(IWorldBridge* bridge, const std::string& libraryName);

	// The library name currently open (empty when none).
	std::string libraryName() const { return libraryName_; }

private slots:
	void onElementSelected(int elementIndex);
	void onSave();

private:
	void loadElement(int elementIndex);

	IWorldBridge* bridge_ = nullptr;
	std::string libraryName_;
	int currentElement_ = -1;
	LibraryTree* tree_ = nullptr;
	PropertyTree* propertyTree_ = nullptr;
	QPushButton* saveButton_ = nullptr;
};