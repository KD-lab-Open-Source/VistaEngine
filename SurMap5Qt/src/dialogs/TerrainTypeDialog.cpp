// TerrainTypeDialog.cpp — see header.

#include "TerrainTypeDialog.h"

#include <vector>

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

TerrainTypeDialog::TerrainTypeDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Terrain Type Names"));
	setMinimumSize(360, 360);

	list_ = new QListWidget(this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Terrain types:"), this));
	layout->addWidget(list_, 1);
	layout->addWidget(buttons);
}

void TerrainTypeDialog::refresh()
{
	if(!bridge_)
		return;
	std::vector<std::string> names;
	std::vector<unsigned> colors;
	bridge_->terrainTypeNames(names, colors);
	list_->clear();
	for(size_t i = 0; i < names.size(); ++i){
		const QString color = (i < colors.size())
			? QString("#%1").arg(colors[i], 6, 16, QLatin1Char('0'))
			: QString();
		list_->addItem(QString("%1. %2  %3").arg((int)i + 1)
			.arg(QString::fromStdString(names[i])).arg(color));
	}
}