// SelectPropertyPanel.h — the Properties dock's panel for the Select tool
// (CSurToolSelect's IDD_BARDLG_SELECT attrib editor).
//
// The original attached the single selected universe object's serializer to an
// AttribEditorCtrl (onSelectionChanged -> SerializerUniverseObject) and showed
// the selection counts for a multi-selection. The Qt port renders the same
// PropertyRow tree through PropertyTree; a multi/empty selection shows the
// counts instead.

#pragma once

#include <QWidget>

class QLabel;
class PropertyTree;
class SelectTool;

class SelectPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit SelectPropertyPanel(QWidget* parent = nullptr);

	void setTool(SelectTool* tool);

	// Re-read the current world selection (onSelectionChanged in the original).
	void refresh();

private:
	void applyEdits();

	SelectTool* tool_ = nullptr;
	QLabel* info_ = nullptr;
	PropertyTree* attrib_ = nullptr;
	bool loading_ = false;
};
