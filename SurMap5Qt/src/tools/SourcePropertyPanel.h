// SourcePropertyPanel.h — the Properties dock's panel for the source-placement
// tool. Lists SourcesLibrary's elements (CSurToolSource's type list) and edits
// the picked element's parameters in a PropertyTree (the original's embedded
// attrib editor over the library element). Qt-side, talks to the engine-free
// SourceTool.

#pragma once

#include <QComboBox>
#include <QWidget>

class PropertyTree;
class SourceTool;

class SourcePropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit SourcePropertyPanel(QWidget* parent = nullptr);

	void setTool(SourceTool* tool);

private:
	void reloadSources();
	void loadElement(int index);
	void applyEdits();

	QComboBox* source_ = nullptr;
	PropertyTree* attrib_ = nullptr;

	SourceTool* tool_ = nullptr;
	bool loading_ = false;   // setRoot in progress — ignore itemChanged
};
