// GeoTxPropertyPanel.h — the Properties dock's panel for the GeoTx tool.
// Qt port of the CSurToolGeoTx dialog's two file pickers (IDC_BTN_BROWSE_FILE
// / IDC_BTN_BROWSE_FILE2). Qt-side, talks to the engine-free GeoTxTool.

#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class GeoTxTool;

class GeoTxPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit GeoTxPropertyPanel(QWidget* parent = nullptr);

	void setTool(GeoTxTool* tool);

private:
	QLineEdit* file1_ = nullptr;
	QLineEdit* file2_ = nullptr;
	QPushButton* browse1_ = nullptr;
	QPushButton* browse2_ = nullptr;

	GeoTxTool* tool_ = nullptr;
};