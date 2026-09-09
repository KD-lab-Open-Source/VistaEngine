// WaveDialog.cpp — see header.

#include "WaveDialog.h"

#include <vector>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

WaveDialog::WaveDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Waves"));
	setMinimumSize(440, 420);

	list_ = new QListWidget(this);

	btnCreate_ = new QPushButton(tr("Create Wave..."), this);
	btnRemove_ = new QPushButton(tr("Remove Wave"), this);
	btnApply_ = new QPushButton(tr("Apply"), this);
	btnRemove_->setEnabled(false);
	btnApply_->setEnabled(false);

	// Wave-line properties (CWaveDlg's DDX fields).
	distanceEdit_ = new QLineEdit(this);
	speedEdit_ = new QLineEdit(this);
	sizeMinEdit_ = new QLineEdit(this);
	sizeMaxEdit_ = new QLineEdit(this);
	generationTimeEdit_ = new QLineEdit(this);
	invertCheck_ = new QCheckBox(tr("Invert"), this);
	for(QLineEdit* e : {distanceEdit_, speedEdit_, sizeMinEdit_, sizeMaxEdit_, generationTimeEdit_})
		e->setEnabled(false);
	invertCheck_->setEnabled(false);

	statusLabel_ = new QLabel(this);
	statusLabel_->setWordWrap(true);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* props = new QGridLayout;
	props->addWidget(new QLabel(tr("Distance:"), this), 0, 0);
	props->addWidget(distanceEdit_, 0, 1);
	props->addWidget(new QLabel(tr("Speed:"), this), 1, 0);
	props->addWidget(speedEdit_, 1, 1);
	props->addWidget(new QLabel(tr("Size min:"), this), 2, 0);
	props->addWidget(sizeMinEdit_, 2, 1);
	props->addWidget(new QLabel(tr("Size max:"), this), 3, 0);
	props->addWidget(sizeMaxEdit_, 3, 1);
	props->addWidget(new QLabel(tr("Generation time:"), this), 4, 0);
	props->addWidget(generationTimeEdit_, 4, 1);
	props->addWidget(invertCheck_, 5, 1);

	auto* btnRow = new QGridLayout;
	btnRow->addWidget(btnCreate_, 0, 0);
	btnRow->addWidget(btnRemove_, 0, 1);
	btnRow->addWidget(btnApply_, 0, 2);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Wave lines:"), this));
	layout->addWidget(list_, 1);
	layout->addLayout(btnRow);
	layout->addLayout(props);
	layout->addWidget(statusLabel_);
	layout->addWidget(buttons);

	connect(btnCreate_, &QPushButton::clicked, this, &WaveDialog::onCreateWave);
	connect(btnRemove_, &QPushButton::clicked, this, &WaveDialog::onRemoveWave);
	connect(btnApply_, &QPushButton::clicked, this, &WaveDialog::onApply);
	connect(list_, &QListWidget::itemSelectionChanged, this, &WaveDialog::onListSelectionChanged);
}

void WaveDialog::setWaves(const QStringList& names)
{
	list_->clear();
	list_->addItems(names);
	onListSelectionChanged();
}

void WaveDialog::refresh()
{
	if(!bridge_)
		return;
	std::vector<std::string> names;
	bridge_->waveNames(names);
	QStringList list;
	for(const std::string& n : names)
		list.push_back(QString::fromStdString(n));
	setWaves(list);
}

void WaveDialog::onListSelectionChanged()
{
	const bool has = list_->currentItem() != nullptr;
	btnRemove_->setEnabled(has);
	btnApply_->setEnabled(has);
	distanceEdit_->setEnabled(has);
	speedEdit_->setEnabled(has);
	sizeMinEdit_->setEnabled(has);
	sizeMaxEdit_->setEnabled(has);
	generationTimeEdit_->setEnabled(has);
	invertCheck_->setEnabled(has);
}

void WaveDialog::onCreateWave()
{
	if(!bridge_){
		statusLabel_->setText(tr("Wave creation needs environment->fixedWaves() (not wired in the Qt port yet)."));
		return;
	}
	// CWaveDlg::OnBnClickedCreateWave: ask for a name, then AddWaves().
	// The original used a separate CEnterNameDlg; here we default the name.
	const QString waveName = tr("Wave %1").arg(list_->count() + 1);
	if(bridge_->createWave(waveName.toStdString())){
		statusLabel_->setText(tr("Wave '%1' created.").arg(waveName));
		refresh();
	}
	else
		statusLabel_->setText(tr("Could not create wave."));
}

void WaveDialog::onRemoveWave()
{
	if(!bridge_){
		statusLabel_->setText(tr("Wave removal needs environment->fixedWaves() (not wired in the Qt port yet)."));
		return;
	}
	const QString name = list_->currentItem() ? list_->currentItem()->text() : QString();
	if(name.isEmpty())
		return;
	if(bridge_->removeWave(name.toStdString())){
		statusLabel_->setText(tr("Wave '%1' removed.").arg(name));
		refresh();
	}
	else
		statusLabel_->setText(tr("Could not remove wave."));
}

void WaveDialog::onApply()
{
	if(!bridge_){
		statusLabel_->setText(tr("Wave apply needs environment->fixedWaves() (not wired in the Qt port yet)."));
		return;
	}
	const QString name = list_->currentItem() ? list_->currentItem()->text() : QString();
	if(name.isEmpty())
		return;
	const float distance = distanceEdit_->text().toFloat();
	const float speed = speedEdit_->text().toFloat();
	const float sizeMin = sizeMinEdit_->text().toFloat();
	const float sizeMax = sizeMaxEdit_->text().toFloat();
	const float genTime = generationTimeEdit_->text().toFloat();
	const bool invert = invertCheck_->isChecked();
	if(bridge_->applyWave(name.toStdString(), distance, speed, sizeMin, sizeMax, genTime, invert))
		statusLabel_->setText(tr("Wave '%1' applied.").arg(name));
	else
		statusLabel_->setText(tr("Could not apply wave."));
}
