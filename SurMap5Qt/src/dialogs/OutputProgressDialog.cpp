// OutputProgressDialog.cpp — see header.

#include "OutputProgressDialog.h"

#include <algorithm>

#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

OutputProgressDialog::OutputProgressDialog(const QString& title, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(title);

	// COutputProgressDlg: two progress bars ranged 0..99 + a file label.
	totalBar_ = new QProgressBar(this);
	totalBar_->setRange(0, 99);
	totalBar_->setValue(0);

	fileBar_ = new QProgressBar(this);
	fileBar_->setRange(0, 99);
	fileBar_->setValue(0);

	fileNameLabel_ = new QLabel(QString(), this);
	fileNameLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* closeButton = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(closeButton, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Total:"), this));
	layout->addWidget(totalBar_);
	layout->addWidget(new QLabel(tr("File:"), this));
	layout->addWidget(fileBar_);
	layout->addWidget(fileNameLabel_);
	layout->addWidget(closeButton);
}

void OutputProgressDialog::progress(int totalPercent, int filePercent, const QString& fileName)
{
	// Clamp to 0..99 like the original (SetPos(min(max(x,0),99))).
	totalBar_->setValue(std::clamp(totalPercent, 0, 99));
	fileBar_->setValue(std::clamp(filePercent, 0, 99));
	fileNameLabel_->setText(fileName);
}
