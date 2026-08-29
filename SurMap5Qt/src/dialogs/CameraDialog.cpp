// CameraDialog.cpp — see header.

#include "CameraDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

CameraDialog::CameraDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Camera Editor"));
	setMinimumSize(420, 380);

	list_ = new QListWidget(this);

	btnCreate_ = new QPushButton(tr("Create Camera..."), this);
	btnDelete_ = new QPushButton(tr("Delete Camera"), this);
	btnPlay_ = new QPushButton(tr("Play Camera"), this);
	btnDelete_->setEnabled(false);
	btnPlay_->setEnabled(false);

	// Spline properties (name / step duration / cycling in the original).
	nameEdit_ = new QLineEdit(this);
	timeEdit_ = new QLineEdit(this);
	nameEdit_->setEnabled(false);
	timeEdit_->setEnabled(false);

	statusLabel_ = new QLabel(this);
	statusLabel_->setWordWrap(true);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* props = new QGridLayout;
	props->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	props->addWidget(nameEdit_, 0, 1);
	props->addWidget(new QLabel(tr("Step duration:"), this), 1, 0);
	props->addWidget(timeEdit_, 1, 1);

	auto* btnRow = new QGridLayout;
	btnRow->addWidget(btnCreate_, 0, 0);
	btnRow->addWidget(btnDelete_, 0, 1);
	btnRow->addWidget(btnPlay_, 0, 2);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Camera paths:"), this));
	layout->addWidget(list_, 1);
	layout->addLayout(btnRow);
	layout->addLayout(props);
	layout->addWidget(statusLabel_);
	layout->addWidget(buttons);

	connect(btnCreate_, &QPushButton::clicked, this, &CameraDialog::onCreateCamera);
	connect(btnDelete_, &QPushButton::clicked, this, &CameraDialog::onDeleteCamera);
	connect(btnPlay_, &QPushButton::clicked, this, &CameraDialog::onPlayCamera);
	connect(list_, &QListWidget::itemSelectionChanged, this, &CameraDialog::onListSelectionChanged);
}

void CameraDialog::setCameras(const QStringList& names)
{
	list_->clear();
	list_->addItems(names);
	onListSelectionChanged();
}

void CameraDialog::onListSelectionChanged()
{
	const bool has = list_->currentItem() != nullptr;
	btnDelete_->setEnabled(has);
	btnPlay_->setEnabled(has);
	nameEdit_->setEnabled(has);
	timeEdit_->setEnabled(has);
	if(has){
		nameEdit_->setText(list_->currentItem()->text());
		timeEdit_->setText(QStringLiteral("1"));
	}
	else{
		nameEdit_->clear();
		timeEdit_->clear();
	}
}

void CameraDialog::onCreateCamera()
{
	// The original asked for a name (CEnterNameDlg) then switched the map
	// editor into CREATE_POINTS mode. cameraManager is not wired in the Qt
	// editor yet.
	statusLabel_->setText(tr("Camera creation needs cameraManager (not wired in the Qt port yet)."));
}

void CameraDialog::onDeleteCamera()
{
	statusLabel_->setText(tr("Camera deletion needs cameraManager (not wired in the Qt port yet)."));
}

void CameraDialog::onPlayCamera()
{
	statusLabel_->setText(tr("Camera replay needs cameraManager (not wired in the Qt port yet)."));
}
