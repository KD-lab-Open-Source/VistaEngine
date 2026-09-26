// UiEditorWindow.h — the Qt UI Editor (port of the MFC UIEditor app).
//
// Layout: screen/control/state tree + the selected node's serialized
// properties on the left, and an embedded PreviewView on the right that the
// engine renders the selected screen into (its own render window, never the
// level's 3D view). Save writes the UI document back to disk.
//
// Qt-clean: talks to the engine only through IWorldBridge.

#pragma once

#include <QMainWindow>

class IWorldBridge;
class PreviewView;
class PropertyTree;
class QTreeWidget;
class QTreeWidgetItem;

class UiEditorWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit UiEditorWindow(IWorldBridge* bridge, QWidget* parent = nullptr);
	~UiEditorWindow() override;

public slots:
	// Rebuild the tree from the bridge (after a change).
	void refresh();

private slots:
	void onTreeSelectionChanged();
	void onPropertyEdited();
	void onSave();
	void onAddControl();
	void onAddState();
	void onDelete();

private:
	int currentNodeId() const;

	IWorldBridge* bridge_ = nullptr;
	QTreeWidget* tree_ = nullptr;
	PropertyTree* properties_ = nullptr;
	PreviewView* preview_ = nullptr;
	bool loading_ = false;
};
