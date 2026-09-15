// TriggerEditorDialog.h — Qt port of TriggerEditor
// (TriggerEditor/TriggerEditor.h).
//
// A modal dialog editing one trigger chain (.scr file): the trigger graph
// canvas in the centre, the action/condition palette + minimap on the left,
// the property tree + condition panel + debugger on the right — the same
// VSplitter/HSplitter arrangement the kdw dialog used, with QSplitter.
//
// Qt-clean: reaches the engine only through RenderViewWidget forwarders
// (the engine-side TriggerSession behind IWorldBridge).

#pragma once

#include <QDialog>

#include <string>

class QAction;
class QSplitter;
class RenderViewWidget;
class TriggerGraphView;
class TriggerClassTree;
class TriggerMiniMapPanel;
class TriggerConditionPanel;
class TriggerDebuggerPanel;
class PropertyTree;
class QPushButton;

class TriggerEditorDialog : public QDialog
{
	Q_OBJECT
public:
	explicit TriggerEditorDialog(RenderViewWidget* view, QWidget* parent = nullptr);
	~TriggerEditorDialog() override;

	// Open the given .scr file (full path) and show the dialog modally.
	// Returns true when the user accepted (the chain was saved).
	bool openChain(const std::string& filePath);

private slots:
	void onGraphSelection();
	void onEditTriggerProps();
	void onEditConditions();
	void onEditAction();
	void onUndo();
	void onRedo();
	void onFind();
	void onSaveProps();

private:
	void reloadAll();
	void loadPropertyPanel();

	RenderViewWidget* view_ = nullptr;
	std::string filePath_;

	TriggerGraphView* graph_ = nullptr;
	TriggerClassTree* classTree_ = nullptr;
	TriggerMiniMapPanel* miniMap_ = nullptr;
	TriggerConditionPanel* conditionPanel_ = nullptr;
	TriggerDebuggerPanel* debugger_ = nullptr;
	PropertyTree* propertyTree_ = nullptr;
	QPushButton* savePropsButton_ = nullptr;

	QAction* actUndo_ = nullptr;
	QAction* actRedo_ = nullptr;
};
