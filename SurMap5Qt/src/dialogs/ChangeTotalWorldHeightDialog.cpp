// ChangeTotalWorldHeightDialog.cpp — see header.

#include "ChangeTotalWorldHeightDialog.h"

#include <algorithm>
#include <cmath>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {
// The engine constants the original used (Terra/terra.h):
//   VX_FRACTION = 5 — fraction bits of a voxel height.
//   MAX_VX_HEIGHT = (1<<(5+9))-1 = 0x3fff.
//   MAX_VX_HEIGHT_WHOLE = 0x3fff >> 5 = 511.
const int kVxFraction = 5;
const int kMaxVxHeight = (1 << (kVxFraction + 9)) - 1;
const int kMaxVxHeightWhole = kMaxVxHeight >> kVxFraction;

// Slider ranges (CDlgChangeTotalWorldHeight statics):
//   MIN_DELTA_H = -128<<VX_FRACTION, MAX_DELTA_H = 128<<VX_FRACTION
//   MIN_SCALE_H = 10, MAX_SCALE_H = 300
const int kMinDeltaH = -128 << kVxFraction;
const int kMaxDeltaH = 128 << kVxFraction;
const int kMinScale = 10;
const int kMaxScale = 300;

// The dialog's histogram widget draws into this many vertical pixels.
const int kHistogramHeight = 120;

// Port of inHistogram2OutHistogram (DlgChangeTotalWorldHeight.cpp): the
// output bin for input bin i is
//   idxout = (minVx + dVxH)>>VX_FRACTION + round((i - minVx>>VX_FRACTION)*kScale)
// then the array is sqrt-normalized exactly like world2Histogram's second half.
void inHistogram2OutHistogram(const int inHist[256], int outHist[256],
                              int minVx, int dVxH, float kScale)
{
	int histArr[256] = {0};
	for(int i = 0; i < 256; i++){
		const int idxout = ((minVx + dVxH) >> kVxFraction)
			+ (int)std::lround((i - (minVx >> kVxFraction)) * kScale);
		if(idxout >= 0 && idxout < 256)
			histArr[idxout] += inHist[i];
	}

	int maxVal = 0;
	for(int i = 0; i < 256; i++)
		if(histArr[i] > maxVal) maxVal = histArr[i];

	for(int i = 0; i < 256; i++){
		int s = 0;
		if(histArr[i]){
			s = (int)std::lround((double)histArr[i] / (double)maxVal * 255.0);
			if(s > kMaxVxHeightWhole) s = kMaxVxHeightWhole;
			if(s < 4) s = 4;
		}
		outHist[i] = s;
	}
}
}

ChangeTotalWorldHeightDialog::ChangeTotalWorldHeightDialog(int minVx, int maxVx,
                                                           const int inputHistogram[256],
                                                           int initialHeight,
                                                           QWidget* parent)
	: QDialog(parent)
	, minVx_(minVx)
	, maxVx_(maxVx)
	, initialHeight_(initialHeight)
{
	setWindowTitle(tr("Change Total World Param"));
	setMinimumSize(520, 360);

	std::copy(inputHistogram, inputHistogram + 256, inputHistogram_);
	deltaVx_ = 0;                // m_deltaH.value=0
	scalePercent_ = 100;         // m_scaleH.value=100

	// --- delta slider: -128..128 voxels ---
	deltaSlider_ = new QSlider(Qt::Horizontal, this);
	deltaSlider_->setRange(kMinDeltaH, kMaxDeltaH);
	deltaSlider_->setValue(deltaVx_);
	deltaValueLabel_ = new QLabel(this);

	// --- scale slider: 10%..300% ---
	scaleSlider_ = new QSlider(Qt::Horizontal, this);
	scaleSlider_->setRange(kMinScale, kMaxScale);
	scaleSlider_->setValue(scalePercent_);
	scaleValueLabel_ = new QLabel(this);

	resizeCheck_ = new QCheckBox(tr("Resize world to new borders"), this);
	resizeCheck_->setChecked(false);

	// The min/max labels the original's OnPaint refreshed (the voxel heights
	// converted to world units via convert_vox2vid — heights in world units).
	const auto fmt = [](int vox){
		// convert_vox2vid: voxel -> world height (Voxel to Vid). The original
		// printed the integer part of vox/32 (whole voxels).
		return QString::number(vox >> kVxFraction);
	};
	inputMinLabel_ = new QLabel(fmt(minVx_), this);
	inputMaxLabel_ = new QLabel(fmt(maxVx_), this);
	outMinLabel_ = new QLabel(this);
	outMaxLabel_ = new QLabel(this);

	updateOutputHistogram();
	onSliderChanged();   // fill outMin/outMax + the value labels

	// --- layout ---
	auto* deltaRow = new QGridLayout;
	deltaRow->addWidget(new QLabel(tr("Delta height:"), this), 0, 0);
	deltaRow->addWidget(deltaSlider_, 0, 1);
	deltaRow->addWidget(deltaValueLabel_, 0, 2);

	auto* scaleRow = new QGridLayout;
	scaleRow->addWidget(new QLabel(tr("Scale:"), this), 0, 0);
	scaleRow->addWidget(scaleSlider_, 0, 1);
	scaleRow->addWidget(scaleValueLabel_, 0, 2);

	auto* sliders = new QVBoxLayout;
	sliders->addLayout(deltaRow);
	sliders->addLayout(scaleRow);
	sliders->addWidget(resizeCheck_);

	auto* slidersBox = new QGroupBox(tr("Parameters"), this);
	slidersBox->setLayout(sliders);

	// Two histograms with their min/max captions. The paint() draws the bars
	// into a fixed-height strip below the captions; the labels sit above.
	auto* inputCaption = new QLabel(tr("Input (min %1, max %2)")
		.arg(inputMinLabel_->text(), inputMaxLabel_->text()), this);
	auto* outputCaption = new QLabel(tr("Output (min %1, max %2)")
		.arg(outMinLabel_->text(), outMaxLabel_->text()), this);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &ChangeTotalWorldHeightDialog::onOk);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(slidersBox);
	layout->addWidget(inputCaption);
	layout->addWidget(outputCaption);
	layout->addStretch(1);   // the paint() histogram strip fills this
	layout->addWidget(buttons);

	connect(deltaSlider_, &QSlider::valueChanged, this, &ChangeTotalWorldHeightDialog::onSliderChanged);
	connect(scaleSlider_, &QSlider::valueChanged, this, &ChangeTotalWorldHeightDialog::onSliderChanged);
	connect(resizeCheck_, &QCheckBox::toggled, this, &ChangeTotalWorldHeightDialog::onSliderChanged);
}

void ChangeTotalWorldHeightDialog::onSliderChanged()
{
	deltaVx_ = deltaSlider_->value();
	scalePercent_ = scaleSlider_->value();
	resizeWorld_ = resizeCheck_->isChecked();
	updateOutputHistogram();

	deltaValueLabel_->setText(tr("%1 vox").arg(deltaVx_ >> kVxFraction));
	scaleValueLabel_->setText(tr("%1%").arg(scalePercent_));

	// Recompute the output min/max (the original did it inside inHistogram2
	// OutHistogram's caller, OnHScroll).
	outMinLabel_->setText(QString::number((minVx_ + deltaVx_) >> kVxFraction));
	outMaxLabel_->setText(QString::number(
		(minVx_ + deltaVx_ + (int)std::lround((maxVx_ - minVx_) * (float)scalePercent_ / 100.f)) >> kVxFraction));

	update();   // repaint the histogram strip
}

void ChangeTotalWorldHeightDialog::updateOutputHistogram()
{
	inHistogram2OutHistogram(inputHistogram_, outHistogram_,
	                         minVx_, deltaSlider_->value(),
	                         (float)scaleSlider_->value() / 100.f);
}

void ChangeTotalWorldHeightDialog::paintEvent(QPaintEvent* /*event*/)
{
	QDialog::paintEvent(nullptr);

	QPainter p(this);

	// Two equal-width strips below the captions. The layout has a stretch
	// between the captions and the buttons; draw the bars there.
	const int stripY = inputMinLabel_ != nullptr ? 0 : 0;   // resolved below
	(void)stripY;
	const int w = width() - 24;
	const int barW = std::max(1, w / 256);

	// The strips sit above the buttons. Compute their tops from the caption
	// label positions: caption Y + caption height + a small gap.
	// Simpler: reserve the top area for sliders (measured), the captions, then
	// draw both strips in the stretch region.
	const QRect slidersRect = deltaSlider_ ? deltaSlider_->geometry() : QRect();
	const int yBase = slidersRect.bottom() + 8;    // below the sliders box
	const int stripHeight = (height() - yBase - 60) / 2;  // leave room for buttons

	// Input strip.
	int y = yBase + 18;
	const int h = std::max(10, stripHeight);
	p.fillRect(8, y, w, h, QColor(96, 96, 96));
	for(int i = 0; i < 256; i++){
		const int colH = inputHistogram_[i] * h / 255;
		p.fillRect(8 + i * barW, y + h - colH, barW, colH, QColor(255, 255, 255));
	}

	// Output strip.
	y += h + 14;
	p.fillRect(8, y, w, h, QColor(96, 96, 96));
	for(int i = 0; i < 256; i++){
		const int colH = outHistogram_[i] * h / 255;
		p.fillRect(8 + i * barW, y + h - colH, barW, colH, QColor(255, 255, 255));
	}
}

void ChangeTotalWorldHeightDialog::onOk()
{
	deltaVx_ = deltaSlider_->value();
	scalePercent_ = scaleSlider_->value();
	resizeWorld_ = resizeCheck_->isChecked();
	accept();
}
