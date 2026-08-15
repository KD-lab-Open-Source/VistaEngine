// ProgressDialog.cpp — see header.

#include "ProgressDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

ProgressDialog::ProgressDialog(const QString& caption, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(caption.isEmpty() ? tr("Progress") : caption);
	// Non-modal: the caller steps the bar from its own loop, so the dialog must
	// not run a nested event loop (that is what PumpMessages did on MFC).
	setModal(false);

	statusLabel_ = new QLabel(tr("Working..."), this);
	percentLabel_ = new QLabel(QStringLiteral("0%"), this);
	percentLabel_->setAlignment(Qt::AlignRight);

	bar_ = new QProgressBar(this);
	bar_->setRange(0, 100);
	bar_->setValue(0);

	stopButton_ = new QPushButton(tr("Stop"), this);
	stopButton_->setEnabled(false);   // armed once the caller allows it

	auto* buttons = new QDialogButtonBox(this);
	buttons->addButton(stopButton_, QDialogButtonBox::RejectRole);
	connect(buttons, &QDialogButtonBox::rejected, this, [this]{
		stopPressed_ = true;
	});

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(statusLabel_);
	layout->addWidget(bar_);
	layout->addWidget(percentLabel_);
	layout->addWidget(buttons);
}

void ProgressDialog::setRange(int lower, int upper)
{
	lower_ = lower;
	upper_ = upper;
	bar_->setRange(0, 100);
	(void)setPos(lower);
}

int ProgressDialog::setPos(int pos)
{
	pos_ = pos;
	bar_->setValue(pos_);
	updatePercent(pos_);
	return pos_;
}

int ProgressDialog::offsetPos(int offset)
{
	return setPos(pos_ + offset);
}

int ProgressDialog::step()
{
	return setPos(pos_ + step_);
}

void ProgressDialog::setStatus(const QString& message)
{
	statusLabel_->setText(message);
}

void ProgressDialog::updatePercent(int pos)
{
	const int divisor = upper_ - lower_;
	int percent = divisor > 0 ? (pos - lower_) * 100 / divisor : 0;
	percent = qBound(0, percent, 100);
	percentLabel_->setText(QStringLiteral("%1%").arg(percent));
}
