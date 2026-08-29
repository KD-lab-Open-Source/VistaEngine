// TexturesStatisticsDialog.cpp — see header.

#include "TexturesStatisticsDialog.h"

#include <QHeaderView>
#include <QLabel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

namespace {
// 1234567 -> "1 234 567" (the original's InsertItem(' ', every 3 digits)).
QString spacedNumber(int value)
{
	QString s = QString::number(value);
	for(int i = s.length() - 3; i > 0; i -= 3)
		s.insert(i, ' ');
	return s;
}
}

TexturesStatisticsDialog::TexturesStatisticsDialog(const QVector<QPair<QString, int>>& rows,
                                                   int totalSize,
                                                   QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Textures Statistics"));
	setMinimumSize(560, 400);

	table_ = new QTableView(this);
	table_->setEditTriggers(QTableView::NoEditTriggers);
	table_->setSelectionBehavior(QTableView::SelectRows);
	table_->horizontalHeader()->setStretchLastSection(true);

	auto* model = new QStandardItemModel(table_);
	model->setColumnCount(2);
	model->setHorizontalHeaderLabels({tr("Name"), tr("Size")});
	for(const auto& row : rows){
		QList<QStandardItem*> items;
		items << new QStandardItem(row.first)
		      << new QStandardItem(spacedNumber(row.second));
		model->appendRow(items);
	}
	table_->setModel(model);

	summaryLabel_ = new QLabel(
		tr("%1 textures, %2 bytes total").arg(rows.size()).arg(spacedNumber(totalSize)),
		this);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(table_);
	layout->addWidget(summaryLabel_);
}
