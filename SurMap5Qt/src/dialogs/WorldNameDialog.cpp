// WorldNameDialog.cpp — see header.

#include "WorldNameDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

WorldNameDialog::WorldNameDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("New world"));

	nameEdit_ = new QLineEdit(tr("Default"), this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Name:"), nameEdit_);
	layout->addRow(buttons);
}

QString WorldNameDialog::worldName() const
{
	return nameEdit_->text();
}

void WorldNameDialog::setWorldName(const QString& name)
{
	nameEdit_->setText(name);
}
