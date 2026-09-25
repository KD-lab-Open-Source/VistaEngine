// ColorPicPropertyPanel.cpp — see header.

#include "ColorPicPropertyPanel.h"

#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>

#include "ColorPicTool.h"

ColorPicPropertyPanel::ColorPicPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	// CSurToolColorPic's controls: the bitmap path + Browse
	// (IDC_EDT_BITMAP/IDC_BTN_BROWSE_FILE), centre alpha (IDC_SLDR_CENTERALPHA
	// 0..255), K colour (IDC_SLD_KCOLOR 0..200), saturation
	// (IDC_SLD_SATURATION 0..200), brightness (IDC_SLD_BRIGHTNESS 0..200), the
	// tint button (IDC_BTN_COLOR) and Put2World (IDCBTN_PUT2ALLWORLD).
	path_ = new QLineEdit(this);
	path_->setReadOnly(true);
	browse_ = new QPushButton(tr("Browse..."), this);

	centerAlpha_ = new QSlider(Qt::Horizontal, this);
	centerAlpha_->setRange(0, 255);
	centerAlpha_->setValue(0);

	kColor_ = new QSlider(Qt::Horizontal, this);
	kColor_->setRange(0, 200);
	kColor_->setValue(0);

	saturation_ = new QSlider(Qt::Horizontal, this);
	saturation_->setRange(0, 200);
	saturation_->setValue(100);

	brightness_ = new QSlider(Qt::Horizontal, this);
	brightness_->setRange(0, 200);
	brightness_->setValue(100);

	color_ = new QPushButton(tr("Tint colour..."), this);
	putAll_ = new QPushButton(tr("Put to all world"), this);

	auto* pathRow = new QHBoxLayout;
	pathRow->addWidget(path_);
	pathRow->addWidget(browse_);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Texture"), pathRow);
	layout->addRow(tr("Center alpha"), centerAlpha_);
	layout->addRow(tr("K colour"), kColor_);
	layout->addRow(tr("Saturation"), saturation_);
	layout->addRow(tr("Brightness"), brightness_);
	layout->addRow(color_);
	layout->addRow(putAll_);

	connect(browse_, &QPushButton::clicked, this, &ColorPicPropertyPanel::browseTexture);
	connect(color_, &QPushButton::clicked, this, &ColorPicPropertyPanel::pickColor);
	connect(putAll_, &QPushButton::clicked, this, &ColorPicPropertyPanel::putToAllWorld);
	connect(centerAlpha_, &QSlider::valueChanged, this, [this](int v){ if(tool_) tool_->setCenterAlpha(v); });
	connect(kColor_, &QSlider::valueChanged, this, [this](int v){ if(tool_) tool_->setKColor(v); });
	connect(saturation_, &QSlider::valueChanged, this, [this](int v){ if(tool_) tool_->setSaturation(v); });
	connect(brightness_, &QSlider::valueChanged, this, [this](int v){ if(tool_) tool_->setBrightness(v); });

	refreshColorButton();
}

void ColorPicPropertyPanel::browseTexture()
{
	// The original opened Resource\TerrainData\Pictures over *.tga.
	QString start = QDir(QDir::currentPath()).filePath("Resource/TerrainData/Pictures");
	if(!QDir(start).exists())
		start = QDir::currentPath();
	const QString file = QFileDialog::getOpenFileName(
		this, tr("Select a terrain texture"), start,
		tr("Targa images (*.tga);;All files (*)"));
	if(file.isEmpty())
		return;
	path_->setText(file);
	if(tool_)
		tool_->setTexturePath(file.toStdString());
}

void ColorPicPropertyPanel::pickColor()
{
	const unsigned rgb = tool_ ? tool_->tintRGB() : 0xFFFFFFu;
	QColor initial((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
	const QColor chosen = QColorDialog::getColor(initial, this, tr("Tint colour"));
	if(!chosen.isValid())
		return;
	if(tool_)
		tool_->setTintRGB((unsigned(chosen.red()) << 16) | (unsigned(chosen.green()) << 8) | unsigned(chosen.blue()));
	refreshColorButton();
}

void ColorPicPropertyPanel::putToAllWorld()
{
	if(!tool_)
		return;
	if(!tool_->putToAllWorld()){
		QMessageBox::warning(this, tr("Put to all world"),
			tr("Select a texture first (the whole world's texture is replaced)."));
		return;
	}
	QMessageBox::information(this, tr("Put to all world"), tr("Texture applied."));
}

void ColorPicPropertyPanel::refreshColorButton()
{
	const unsigned rgb = tool_ ? tool_->tintRGB() : 0xFFFFFFu;
	color_->setStyleSheet(QString("background-color: rgb(%1,%2,%3);")
		.arg((rgb >> 16) & 0xff).arg((rgb >> 8) & 0xff).arg(rgb & 0xff));
}

void ColorPicPropertyPanel::setTool(ColorPicTool* tool)
{
	tool_ = tool;
	if(!tool_)
		return;
	path_->setText(QString::fromStdString(tool_->texturePath()));
	centerAlpha_->setValue(tool_->centerAlpha());
	kColor_->setValue(tool_->kColor());
	saturation_->setValue(tool_->saturation());
	brightness_->setValue(tool_->brightness());
	refreshColorButton();
}
