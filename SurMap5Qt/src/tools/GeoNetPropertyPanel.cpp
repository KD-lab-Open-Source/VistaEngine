// GeoNetPropertyPanel.cpp — see header.

#include "GeoNetPropertyPanel.h"

#include <QFormLayout>
#include <QLabel>

#include "GeoNetTool.h"

GeoNetPropertyPanel::GeoNetPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	// CSurToolGeoNet's sliders: height [0, MAX_VX_HEIGHT], noise, mesh
	// density [20, 4000]. The original used CEScrollVx/CVoxVarWin/CIntVarWin;
	// QSlider is the Qt equivalent.
	height_ = new QSlider(Qt::Horizontal, this);
	height_->setRange(0, 0x3fff);   // MAX_VX_HEIGHT
	height_->setValue(64);

	noise_ = new QSlider(Qt::Horizontal, this);
	noise_->setRange(0, 100);
	noise_->setValue(100);

	mesh_ = new QSlider(Qt::Horizontal, this);
	mesh_->setRange(20, 4000);
	mesh_->setValue(1600);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Height"), height_);
	layout->addRow(tr("Noise"), noise_);
	layout->addRow(tr("Mesh"), mesh_);

	connect(height_, &QSlider::valueChanged, this, [this](int v){
		if(tool_) tool_->setHeight(v);
	});
	connect(noise_, &QSlider::valueChanged, this, [this](int v){
		if(tool_) tool_->setNoise(v);
	});
	connect(mesh_, &QSlider::valueChanged, this, [this](int v){
		if(tool_) tool_->setMesh(v);
	});
}

void GeoNetPropertyPanel::setTool(GeoNetTool* tool)
{
	tool_ = tool;
	if(!tool_)
		return;
	height_->setValue(tool_->height());
	noise_->setValue(tool_->noise());
	mesh_->setValue(tool_->mesh());
}