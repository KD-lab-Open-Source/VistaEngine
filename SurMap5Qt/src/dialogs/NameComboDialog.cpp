// NameComboDialog.cpp — see header.

#include "NameComboDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

NameComboDialog::NameComboDialog(const QString& title, const QString& name,
                                 const QString& combo, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(title);

	combo_ = new QComboBox(this);
	combo_->setEditable(true);

	// Original filled the combo from a semicolon-separated list (ComboStrings
	// / splitComboList); a plain split matches that format.
	const QStringList items = combo.split(';', Qt::SkipEmptyParts);
	for(const QString& item : items)
		combo_->addItem(item.trimmed());
	combo_->setCurrentText(name);
	// The initial name is selected for quick overwrite (SetEditSel(0,-1)).
	combo_->lineEdit()->selectAll();
	combo_->setFocus();

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Name:"), combo_);
	layout->addRow(buttons);
}

QString NameComboDialog::name() const
{
	return combo_->currentText();
}
