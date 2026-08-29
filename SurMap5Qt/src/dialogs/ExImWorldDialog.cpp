// ExImWorldDialog.cpp — see header.

#include "ExImWorldDialog.h"

#include <QAbstractItemModel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QModelIndex>
#include <QProcess>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "editor/WorldList.h"

int ExImWorldDialog::s_previsionWorldSelect = 0;

ExImWorldDialog::ExImWorldDialog(const QString& path2worlds,
                                 const QString& packerPath,
                                 QWidget* parent)
	: QDialog(parent)
	, worldList_(new WorldList)
	, path2worlds_(path2worlds)
	, packerPath_(packerPath.isEmpty() ? QStringLiteral("packer.exe") : packerPath)
{
	setWindowTitle(tr("Export/Import World"));
	setMinimumSize(520, 360);

	table_ = new QTableView(this);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->horizontalHeader()->setStretchLastSection(true);

	auto* model = new QStandardItemModel(table_);
	model->setColumnCount(3);
	model->setHorizontalHeaderLabels({tr("Name"), tr("Interface"), tr("Size")});
	table_->setModel(model);

	// Full export: pack the whole world (packer " e"). Partial export: "ex".
	btnExport_ = new QPushButton(tr("Export..."), this);
	btnImport_ = new QPushButton(tr("Import..."), this);
	btnImport_->setDefault(true);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	buttons->addButton(btnExport_, QDialogButtonBox::ActionRole);
	buttons->addButton(btnImport_, QDialogButtonBox::ActionRole);

	statusLabel_ = new QLabel(this);
	statusLabel_->setWordWrap(true);

	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(btnExport_, &QPushButton::clicked, this, &ExImWorldDialog::onExport);
	connect(btnImport_, &QPushButton::clicked, this, &ExImWorldDialog::onImport);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(table_);
	layout->addWidget(statusLabel_);
	layout->addWidget(buttons);

	fillWorldList();
}

ExImWorldDialog::~ExImWorldDialog()
{
	delete worldList_;
}

void ExImWorldDialog::fillWorldList()
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

	// CDlgExImWorld::fillWorldList restored the previously selected row.
	const int count = model->rowCount();
	int index = s_previsionWorldSelect;
	if(index >= count) index = count - 1;
	if(index < 0) index = 0;
	if(count > 0){
		table_->selectRow(index);
		table_->scrollTo(model->index(index, 0));
	}
}

QString ExImWorldDialog::currentWorldName() const
{
	const QModelIndex sel = table_->currentIndex();
	if(!sel.isValid())
		return QString();
	return sel.sibling(sel.row(), 0).data().toString();
}

void ExImWorldDialog::onExport()
{
	// CDlgExImWorld::OnButtonExport: pick a .pkw path, then run
	// packer " e" <path> <world> (full) or " ex" (partial).
	const QString world = currentWorldName();
	if(world.isEmpty())
		return;

	QString path = QFileDialog::getSaveFileName(this, tr("Export world..."),
	                                           QString(), tr("(*.pkw)|*.pkw"));
	if(path.isEmpty())
		return;

	statusLabel_->setText(tr("Exporting %1...").arg(world));
	QProcess proc(this);
	// The original passed the command line as one argument to CreateProcessEx;
	// QProcess keeps the pieces separate (no quoting surprises).
	proc.start(packerPath_, {QStringLiteral("e"), path, world});
	proc.waitForFinished(-1);
	if(proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
		statusLabel_->setText(tr("Export failed (%1).").arg(QString::fromLocal8Bit(proc.readAllStandardError())));
	else
		statusLabel_->setText(tr("Exported to %1.").arg(path));
}

void ExImWorldDialog::onImport()
{
	// CDlgExImWorld::OnButtonImport: pick a .pkw, run packer " i" <path>,
	// then refresh the list.
	QString path = QFileDialog::getOpenFileName(this, tr("Import world..."),
	                                            QString(), tr("(*.pkw)|*.pkw"));
	if(path.isEmpty())
		return;

	statusLabel_->setText(tr("Importing..."));
	QProcess proc(this);
	proc.start(packerPath_, {QStringLiteral("i"), path});
	proc.waitForFinished(-1);
	if(proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
		statusLabel_->setText(tr("Import failed (%1).").arg(QString::fromLocal8Bit(proc.readAllStandardError())));
	else
		statusLabel_->setText(tr("Imported."));
	fillWorldList();
}
