// AnchorPropertyPanel.h — the Properties dock's panel for the anchor-placement
// tool. Edits the single editable Anchor's parameters (CSurToolAnchor's
// embedded attrib editor) through the bridge's PropertyRow tree.

#pragma once

#include <QWidget>

class PropertyTree;
class AnchorTool;

class AnchorPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit AnchorPropertyPanel(QWidget* parent = nullptr);

	void setTool(AnchorTool* tool);

private:
	void load();
	void applyEdits();

	PropertyTree* attrib_ = nullptr;

	AnchorTool* tool_ = nullptr;
	bool loading_ = false;   // setRoot in progress — ignore itemChanged
};
