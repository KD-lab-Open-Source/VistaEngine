// EnterNameDialog.cpp — see header.

#include "EnterNameDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

EnterNameDialog::EnterNameDialog(const QString& initial, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Enter name"));

	edit_ = new QLineEdit(initial, this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Name:"), edit_);
	layout->addRow(buttons);
}

QString EnterNameDialog::text() const
{
	return edit_->text();
}
