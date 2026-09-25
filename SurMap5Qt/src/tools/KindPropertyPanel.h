// KindPropertyPanel.h — the Properties dock's panel for the surface-kind
// brush. Qt port of CSurToolKind's dialog: the 16 surface-type radio buttons
// (IDC_RDB_INSDELHARDNESS_SETSURKIND_...1..16) and the colour swatch
// (IDC_COLORTERRAINTYPE). Qt-side, talks to the engine-free KindTool.

#pragma once

#include <QComboBox>
#include <QWidget>

class KindTool;
class IWorldBridge;

class KindPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit KindPropertyPanel(QWidget* parent = nullptr);

	void setTool(KindTool* tool);
	// Fill the type list from the bridge (TerrainTypeDescriptor names/colors).
	void setBridge(IWorldBridge* bridge);

private:
	void reloadTypes();

	QComboBox* kind_ = nullptr;

	KindTool* tool_ = nullptr;
	IWorldBridge* bridge_ = nullptr;
};
