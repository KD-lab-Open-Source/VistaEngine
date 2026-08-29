// TexturesStatisticsDialog.h — Qt port of CDlgTexturesStatistics
// (SurMap5/DlgTexturesStatistics.{h,cpp}).
//
// A read-only table of the loaded texture library: name + size in bytes, with
// the texture count and the summed size below. The data comes from
// RenderViewWidget::textureStatistics (the engine's GetTexLibrary).
#pragma once

#include <QDialog>

#include <QVector>

class QLabel;
class QTableView;

class TexturesStatisticsDialog : public QDialog
{
	Q_OBJECT
public:
	// rows: name+size pairs (RenderViewWidget::textureStatistics output).
	// totalSize: the summed size in bytes.
	TexturesStatisticsDialog(const QVector<QPair<QString, int>>& rows,
	                         int totalSize,
	                         QWidget* parent = nullptr);

private:
	QTableView* table_ = nullptr;
	QLabel* summaryLabel_ = nullptr;
};
