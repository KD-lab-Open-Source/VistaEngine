// CreateAttribDialog.cpp — see header.

#include "CreateAttribDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

CreateAttribDialog::CreateAttribDialog(bool allowPaste, bool pasteByDefault,
                                       const QString& title,
                                       const QString& defaultName,
                                       QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(title);

	// CCreateAttribDlg: defaultName_ = oldName ? oldName : title.
	nameEdit_ = new QLineEdit(defaultName.isEmpty() ? title : defaultName, this);
	nameEdit_->selectAll();
	nameEdit_->setFocus();

	pasteCheck_ = new QCheckBox(tr("Paste"), this);
	pasteCheck_->setEnabled(allowPaste);
	pasteCheck_->setChecked(pasteByDefault);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Name:"), nameEdit_);
	layout->addRow(pasteCheck_);
	layout->addRow(buttons);
}

QString CreateAttribDialog::name() const
{
	return nameEdit_->text();
}

bool CreateAttribDialog::paste() const
{
	return pasteCheck_->isChecked();
}
