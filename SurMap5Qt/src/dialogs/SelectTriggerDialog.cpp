// SelectTriggerDialog.cpp — see header.

#include "SelectTriggerDialog.h"

#include <algorithm>
#include <cstdio>

#include <QDir>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "dialogs/WorldNameDialog.h"

SelectTriggerDialog::SelectTriggerDialog(const QString& path2triggersFiles,
                                         const QString& title, QWidget* parent)
	: QDialog(parent)
	, path2triggersFiles_(path2triggersFiles)
	, title_(title)
{
	setWindowTitle(title_);

	list_ = new QListWidget(this);
	list_->setSelectionMode(QAbstractItemView::SingleSelection);

	btnNew_ = new QPushButton(tr("New..."), this);
	btnDelete_ = new QPushButton(tr("Delete"), this);
	btnCopy_ = new QPushButton(tr("Copy..."), this);
	connect(btnNew_, &QPushButton::clicked, this, &SelectTriggerDialog::onNewTrigger);
	connect(btnDelete_, &QPushButton::clicked, this, &SelectTriggerDialog::onDeleteTrigger);
	connect(btnCopy_, &QPushButton::clicked, this, &SelectTriggerDialog::onCopyTrigger);
	connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item){
		selectedTrigger_ = item->text();
		accept();
	});

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &SelectTriggerDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* btnLayout = new QVBoxLayout;
	btnLayout->addWidget(btnNew_);
	btnLayout->addWidget(btnDelete_);
	btnLayout->addWidget(btnCopy_);
	btnLayout->addStretch(1);

	auto* layout = new QHBoxLayout;
	layout->addWidget(list_, 1);
	layout->addLayout(btnLayout);

	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(layout);
	mainLayout->addWidget(buttons);

	fillTriggerList();
}

void SelectTriggerDialog::fillTriggerList()
{
	// CDlgSelectTrigger::fillTriggerList: every *.scr file in the dir.
	list_->clear();
	const QDir dir(path2triggersFiles_);
	const QStringList files = dir.entryList(QStringList("*.scr"), QDir::Files, QDir::Name);
	for(const QString& file : files)
		list_->addItem(file);
}

QString SelectTriggerDialog::askForName()
{
	// Both New and Copy used CDlgWorldName for the new name.
	WorldNameDialog dlg(this);
	dlg.setWorldName(tr("Default"));
	if(dlg.exec() != QDialog::Accepted)
		return QString();
	return dlg.worldName();
}

void SelectTriggerDialog::onNewTrigger()
{
	// OnBnClickedBtnNewtrigger: ask for a name, accept with it directly.
	const QString name = askForName();
	if(name.isEmpty())
		return;
	selectedTrigger_ = name;
	accept();
}

void SelectTriggerDialog::onDeleteTrigger()
{
	// OnBnClickedBtnDeletetrigger: confirm, delete the file, refresh.
	QListWidgetItem* item = list_->currentItem();
	if(!item)
		return;
	const QString name = item->text();
	const QString file = path2triggersFiles_ + "/" + name;
	if(QMessageBox::question(this, tr("Delete"),
	                         tr("Really want to remove trigger - %1 ?").arg(name),
	                         QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
		return;
	QFile::remove(file);
	fillTriggerList();
}

void SelectTriggerDialog::onCopyTrigger()
{
	// OnCopyTriggerButton: ask for a name, copy the selected file, accept.
	QListWidgetItem* item = list_->currentItem();
	if(!item)
		return;
	QString name = askForName();
	if(name.isEmpty())
		return;
	if(name.size() < 4 || !name.endsWith(".scr", Qt::CaseInsensitive))
		name += ".scr";
	const QString source = path2triggersFiles_ + "/" + item->text();
	const QString destination = path2triggersFiles_ + "/" + name;
	if(!QFile::copy(source, destination)){
		QMessageBox::warning(this, tr("Error"), tr("Unable to copy trigger!"));
		selectedTrigger_.clear();
		return;
	}
	selectedTrigger_ = name;
	accept();
}

void SelectTriggerDialog::accept()
{
	// CDlgSelectTrigger::OnOK: accept only when a row is selected.
	QListWidgetItem* item = list_->currentItem();
	if(item){
		selectedTrigger_ = item->text();
		QDialog::accept();
	}
	else{
		QMessageBox::warning(this, tr("Warning"), tr("Select a trigger first"));
	}
}
