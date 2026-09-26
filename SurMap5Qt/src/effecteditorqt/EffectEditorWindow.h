// EffectEditorWindow.h — the Qt Effects Editor (port of the MFC EffectEditor).
//
// First slice: open a .effect file (IWorldBridge::effectOpen), show the
// effect → emitter → curve tree and the selected node's serialized properties
// (PropertyTree), and save back (effectSave/effectSaveAs). The 3D preview and
// the curve editor are later milestones.
//
// Qt-clean: talks to the engine only through IWorldBridge.

#pragma once

#include <QMainWindow>

#include <vector>

class IWorldBridge;
class PropertyTree;
class QTableWidget;
class QTreeWidget;

class EffectEditorWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit EffectEditorWindow(IWorldBridge* bridge, QWidget* parent = nullptr);
	~EffectEditorWindow() override;

public slots:
	void refresh();

private slots:
	void onOpen();
	void onSave();
	void onSaveAs();
	void onTreeSelectionChanged();
	void onPropertyEdited();
	void onCurveKeyChanged(int row, int column);

private:
	void populateCurveKeys(int curveNodeId);

	IWorldBridge* bridge_ = nullptr;
	QTreeWidget* tree_ = nullptr;
	PropertyTree* properties_ = nullptr;
	QTableWidget* curveKeys_ = nullptr;
	int curveNodeId_ = -1;
	bool loading_ = false;
};
