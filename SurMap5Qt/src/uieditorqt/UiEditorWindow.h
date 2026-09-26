// UiEditorWindow.h — the Qt UI Editor (port of the MFC UIEditor app).
//
// First slice: the screen/control/state tree (IWorldBridge::uiTree) on the
// left and the selected node's serialized properties (PropertyTree) on the
// right, with Save writing the UI document back to disk (uiSave). The live
// render preview is a later milestone.
//
// Qt-clean: talks to the engine only through IWorldBridge.

#pragma once

#include <QMainWindow>

#include <vector>

class IWorldBridge;
class PropertyTree;
class QCheckBox;
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
	void onPreviewToggled(bool on);

private:
	int currentNodeId() const;

	IWorldBridge* bridge_ = nullptr;
	QTreeWidget* tree_ = nullptr;
	PropertyTree* properties_ = nullptr;
	QCheckBox* previewCheck_ = nullptr;
	bool loading_ = false;
};
