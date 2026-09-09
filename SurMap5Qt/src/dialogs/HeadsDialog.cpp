// HeadsDialog.cpp — see header.

#include "HeadsDialog.h"

#include <vector>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

HeadsDialog::HeadsDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Heads"));
	setMinimumSize(360, 360);

	list_ = new QListWidget(this);
	edit_ = new QLineEdit(this);
	btnAdd_ = new QPushButton(tr("Add"), this);
	btnRemove_ = new QPushButton(tr("Remove"), this);
	btnSave_ = new QPushButton(tr("Save"), this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* editRow = new QGridLayout;
	editRow->addWidget(new QLabel(tr("File name:"), this), 0, 0);
	editRow->addWidget(edit_, 0, 1);
	editRow->addWidget(btnAdd_, 0, 2);

	auto* btnRow = new QGridLayout;
	btnRow->addWidget(btnRemove_, 0, 0);
	btnRow->addWidget(btnSave_, 0, 1);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Head files:"), this));
	layout->addWidget(list_, 1);
	layout->addLayout(editRow);
	layout->addLayout(btnRow);
	layout->addWidget(buttons);

	connect(btnAdd_, &QPushButton::clicked, this, &HeadsDialog::onAdd);
	connect(btnRemove_, &QPushButton::clicked, this, &HeadsDialog::onRemove);
	connect(btnSave_, &QPushButton::clicked, this, &HeadsDialog::onSave);
}

void HeadsDialog::refresh()
{
	if(!bridge_)
		return;
	std::vector<std::string> names;
	bridge_->headNames(names);
	list_->clear();
	for(const std::string& n : names)
		list_->addItem(QString::fromStdString(n));
}

void HeadsDialog::onAdd()
{
	const QString name = edit_->text().trimmed();
	if(name.isEmpty())
		return;
	list_->addItem(name);
	edit_->clear();
}

void HeadsDialog::onRemove()
{
	if(list_->currentItem())
		delete list_->currentItem();
}

void HeadsDialog::onSave()
{
	if(!bridge_)
		return;
	std::vector<std::string> names;
	for(int i = 0; i < list_->count(); ++i)
		names.push_back(list_->item(i)->text().toStdString());
	if(bridge_->setHeadNames(names))
		accept();   // close on success
}