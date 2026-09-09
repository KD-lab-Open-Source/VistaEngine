// CameraDialog.cpp — see header.

#include "CameraDialog.h"

#include <vector>

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

void CameraDialog::refresh()
{
	// Reload the camera list from the bridge (CameraManager::splines names).
	if(!bridge_)
		return;
	std::vector<std::string> names;
	bridge_->cameraNames(names);
	QStringList list;
	for(const std::string& n : names)
		list.push_back(QString::fromStdString(n));
	setCameras(list);
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
	// CCameraDlg::OnBnClickedButton1: ask for a name, then register a new
	// camera spline (the original switched to CREATE_POINTS mode; here the
	// empty spline is created and the list refreshed).
	if(!bridge_){
		statusLabel_->setText(tr("Camera creation needs cameraManager (not wired in the Qt port yet)."));
		return;
	}
	const QString name = nameEdit_->text().trimmed();
	if(name.isEmpty()){
		statusLabel_->setText(tr("Enter a camera name first."));
		return;
	}
	if(bridge_->createCamera(name.toStdString())){
		statusLabel_->setText(tr("Camera '%1' created.").arg(name));
		refresh();
	}
	else
		statusLabel_->setText(tr("Could not create camera."));
}

void CameraDialog::onDeleteCamera()
{
	if(!bridge_){
		statusLabel_->setText(tr("Camera deletion needs cameraManager (not wired in the Qt port yet)."));
		return;
	}
	const QString name = nameEdit_->text().trimmed();
	if(name.isEmpty())
		return;
	if(bridge_->deleteCamera(name.toStdString())){
		statusLabel_->setText(tr("Camera '%1' deleted.").arg(name));
		refresh();
	}
	else
		statusLabel_->setText(tr("Could not delete camera."));
}

void CameraDialog::onPlayCamera()
{
	if(!bridge_){
		statusLabel_->setText(tr("Camera replay needs cameraManager (not wired in the Qt port yet)."));
		return;
	}
	const QString name = nameEdit_->text().trimmed();
	if(name.isEmpty())
		return;
	if(bridge_->playCamera(name.toStdString()))
		statusLabel_->setText(tr("Replaying camera '%1'.").arg(name));
	else
		statusLabel_->setText(tr("Could not replay camera."));
}
