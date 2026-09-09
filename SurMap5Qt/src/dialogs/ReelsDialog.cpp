// ReelsDialog.cpp — see header.

#include "ReelsDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ReelsDialog::ReelsDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Cut-Scenes (Reels)"));
	setMinimumSize(360, 360);

	list_ = new QListWidget(this);
	btnRefresh_ = new QPushButton(tr("Refresh"), this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Video files (Resource\\Video):"), this));
	layout->addWidget(list_, 1);
	layout->addWidget(btnRefresh_);
	layout->addWidget(buttons);

	connect(btnRefresh_, &QPushButton::clicked, this, &ReelsDialog::onRefresh);

	onRefresh();
}

void ReelsDialog::onRefresh()
{
	// CFileLibraryEditorDlg("Resource\\Video", "*.bik"): list the .bik files
	// in the video directory next to the executable.
	list_->clear();
	const QString dir = QDir(QCoreApplication::applicationDirPath())
		.absoluteFilePath(QStringLiteral("Resource\\Video"));
	QDir d(dir);
	const QStringList entries = d.entryList(QStringList{ QStringLiteral("*.bik") },
		QDir::Files | QDir::NoDotAndDotDot);
	for(const QString& e : entries)
		list_->addItem(e);
	if(list_->count() == 0)
		list_->addItem(tr("(no .bik files found in %1)").arg(dir));
}