// WorldPropertiesDialog.cpp — see header.

#include "WorldPropertiesDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
// The "size" names the original showed (getEnumNameAlt over SIZE_POWER:
// VMAP.CPP registers "512".."8192"). Powers map to 1<<power vertices.
QString sizeName(int power)
{
	switch(power){
	case 7:  return QStringLiteral("128");
	case 8:  return QStringLiteral("256");
	case 9:  return QStringLiteral("512");
	case 10: return QStringLiteral("1024");
	case 11: return QStringLiteral("2048");
	case 12: return QStringLiteral("4096");
	case 13: return QStringLiteral("8192");
	default: return QString::number(1 << power);
	}
}

QString methodName(int method)
{
	// vrtMapCreationParam::eCreateWorldMetod: FullPlain / Mountains.
	switch(method){
	case 0: return QStringLiteral("Full Plain");
	case 1: return QStringLiteral("Mountains");
	default: return QString::number(method);
	}
}
}

WorldPropertiesDialog::WorldPropertiesDialog(int hSize, int vSize,
                                             int hSizePower, int vSizePower,
                                             int createMethod, int initialHeight,
                                             QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("World Properties"));

	// The original's single line: "512x512" (getEnumNameAlt over both powers).
	// Add the vertex count — the number the map tools actually work with.
	sizeLabel_ = new QLabel(
		tr("%1x%2 (%3x%4 vertices)")
			.arg(sizeName(hSizePower), sizeName(vSizePower))
			.arg(hSize).arg(vSize),
		this);

	auto* methodLabel = new QLabel(methodName(createMethod), this);
	auto* heightLabel = new QLabel(QString::number(initialHeight), this);

	auto* form = new QFormLayout;
	form->addRow(tr("Map size:"), sizeLabel_);
	form->addRow(tr("Generation method:"), methodLabel);
	form->addRow(tr("Initial height:"), heightLabel);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

	auto* layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);
}
