// GeoTxPropertyPanel.cpp — see header.

#include "GeoTxPropertyPanel.h"

#include <QFileDialog>
#include <QFormLayout>

#include "GeoTxTool.h"

GeoTxPropertyPanel::GeoTxPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	file1_ = new QLineEdit(this);
	file2_ = new QLineEdit(this);
	browse1_ = new QPushButton(tr("Browse..."), this);
	browse2_ = new QPushButton(tr("Browse..."), this);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Texture 1"), file1_);
	layout->addRow(QString(), browse1_);
	layout->addRow(tr("Texture 2"), file2_);
	layout->addRow(QString(), browse2_);

	// CSurToolGeoTx::OnBnClickedBtnBrowseFile: pick a .tga from the terrain
	// textures directory.
	connect(browse1_, &QPushButton::clicked, this, [this]{
		const QString file = QFileDialog::getOpenFileName(this, tr("Select geo texture"),
			QString(), tr("Textures (*.tga)"));
		if(!file.isEmpty()){
			file1_->setText(file);
			if(tool_) tool_->setTextureFile(file.toStdString());
		}
	});
	connect(browse2_, &QPushButton::clicked, this, [this]{
		const QString file = QFileDialog::getOpenFileName(this, tr("Select geo texture"),
			QString(), tr("Textures (*.tga)"));
		if(!file.isEmpty()){
			file2_->setText(file);
			if(tool_) tool_->setTextureFile2(file.toStdString());
		}
	});
}

void GeoTxPropertyPanel::setTool(GeoTxTool* tool)
{
	tool_ = tool;
	if(!tool_)
		return;
	file1_->setText(QString::fromStdString(tool_->textureFile()));
	file2_->setText(QString::fromStdString(tool_->textureFile2()));
}