// SelectWorldDialog.cpp — see header.

#include "SelectWorldDialog.h"

#include <QAbstractItemModel>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "editor/WorldList.h"
#include "dialogs/EnterNameDialog.h"
#include "dialogs/WorldNameDialog.h"

int SelectWorldDialog::s_previsionWorldSelect = 0;

SelectWorldDialog::SelectWorldDialog(const QString& path2worlds,
                                     const QString& title,
                                     bool enableCreateDir,
                                     QWidget* parent)
	: QDialog(parent)
	, worldList_(new WorldList)
	, path2worlds_(path2worlds)
	, title_(title)
	, enableCreateDir_(enableCreateDir)
{
	setWindowTitle(title_);
	setMinimumSize(480, 320);

	table_ = new QTableView(this);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->horizontalHeader()->setStretchLastSection(true);

	auto* model = new QStandardItemModel(table_);
	model->setColumnCount(3);
	model->setHorizontalHeaderLabels({tr("Name"), tr("Interface"), tr("Size")});
	table_->setModel(model);

	auto* btnNew = new QPushButton(tr("New..."), this);
	auto* btnDelete = new QPushButton(tr("Delete"), this);
	auto* btnRename = new QPushButton(tr("Rename..."), this);
	if(!enableCreateDir_)
		btnNew->setVisible(false);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->addButton(btnNew, QDialogButtonBox::ActionRole);
	buttons->addButton(btnDelete, QDialogButtonBox::ActionRole);
	buttons->addButton(btnRename, QDialogButtonBox::ActionRole);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(btnNew, &QPushButton::clicked, this, &SelectWorldDialog::onNewWorld);
	connect(btnDelete, &QPushButton::clicked, this, &SelectWorldDialog::onDeleteWorld);
	connect(btnRename, &QPushButton::clicked, this, &SelectWorldDialog::onRenameWorld);
	connect(table_, &QTableView::doubleClicked, this, &SelectWorldDialog::onDoubleClicked);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(table_);
	layout->addWidget(buttons);

	fillWorldList();
}

SelectWorldDialog::~SelectWorldDialog()
{
	delete worldList_;
}

void SelectWorldDialog::fillWorldList()
{
	worldList_->scan(path2worlds_.toStdString(), /*userWorlds=*/false);

	auto* model = qobject_cast<QStandardItemModel*>(table_->model());
	model->removeRows(0, model->rowCount());

	const auto& worlds = worldList_->worlds();
	for(size_t i = 0; i < worlds.size(); ++i){
		const auto& w = worlds[i];
		QList<QStandardItem*> row;
		row << new QStandardItem(QString::fromStdString(w.name))
			<< new QStandardItem(QString::fromStdString(w.interfaceName))
			<< new QStandardItem(QStringLiteral("%1x%2").arg(w.sizeX).arg(w.sizeY));
		model->appendRow(row);
	}
	selectPrevious();
}

void SelectWorldDialog::selectPrevious()
{
	auto* model = qobject_cast<QStandardItemModel*>(table_->model());
	const int count = model->rowCount();
	int index = s_previsionWorldSelect;
	if(index >= count) index = count - 1;
	if(index < 0) index = 0;
	if(count > 0){
		table_->selectRow(index);
		table_->scrollTo(model->index(index, 0));
	}
}

void SelectWorldDialog::onNewWorld()
{
	// CDlgSelectWorld::OnButtonNew: ask for a name, create the directory.
	WorldNameDialog dlg(this);
	dlg.setWorldName(tr("Default"));
	if(dlg.exec() != QDialog::Accepted)
		return;
	const QString name = dlg.worldName();
	if(WorldList::createWorldDir(path2worlds_.toStdString(), name.toStdString())){
		selectedWorld_ = name;
		accept();
	}
	else{
		QMessageBox::warning(this, tr("Error"), tr("Could not create world: %1").arg(name));
	}
}

void SelectWorldDialog::onDeleteWorld()
{
	// CDlgSelectWorld::OnButtonDelete: confirm, delete the world's files.
	const QModelIndex sel = table_->currentIndex();
	if(!sel.isValid())
		return;
	const QString name = sel.sibling(sel.row(), 0).data().toString();
	if(name.isEmpty())
		return;
	if(QMessageBox::question(this, tr("Delete"), tr("Really delete world %1?").arg(name),
	                         QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
		return;
	WorldList::deleteWorld(path2worlds_.toStdString(), name.toStdString());
	fillWorldList();
}

void SelectWorldDialog::onRenameWorld()
{
	// CDlgSelectWorld::OnButtonRename: ask for the new name, rename the files.
	const QModelIndex sel = table_->currentIndex();
	if(!sel.isValid())
		return;
	const QString oldName = sel.sibling(sel.row(), 0).data().toString();
	if(oldName.isEmpty())
		return;
	EnterNameDialog dlg(oldName, this);
	if(dlg.exec() != QDialog::Accepted)
		return;
	const QString newName = dlg.text();
	if(newName.isEmpty() || newName == oldName)
		return;
	if(!WorldList::renameWorld(path2worlds_.toStdString(), oldName.toStdString(), newName.toStdString())){
		QMessageBox::warning(this, tr("Rename"), tr("Could not rename %1 to %2").arg(oldName, newName));
		return;
	}
	fillWorldList();
}

void SelectWorldDialog::onDoubleClicked(const QModelIndex& index)
{
	// CDlgSelectWorld::OnListDoubleClick: accept with the clicked row.
	selectedWorld_ = index.sibling(index.row(), 0).data().toString();
	s_previsionWorldSelect = index.row();
	accept();
}

void SelectWorldDialog::accept()
{
	// CDlgSelectWorld::OnOK: the selection mark decides accept vs cancel.
	const QModelIndex sel = table_->currentIndex();
	if(sel.isValid()){
		selectedWorld_ = sel.sibling(sel.row(), 0).data().toString();
		s_previsionWorldSelect = sel.row();
		QDialog::accept();
	}
	else{
		QDialog::reject();
	}
}
