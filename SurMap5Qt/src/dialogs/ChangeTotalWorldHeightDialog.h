// ChangeTotalWorldHeightDialog.h — Qt port of CDlgChangeTotalWorldHeight
// (SurMap5/DlgChangeTotalWorldHeight.{h,cpp}).
//
// Two sliders reshape the world's terrain: delta shifts every vertex's height
// by a fixed amount, scale multiplies the height range (delta + min + round(
// (h-min)*kScale), clamped to MAX_VX_HEIGHT). Two live histograms show the
// before/after height distribution. The original also hosted an attributes
// editor for vrtMapCreationParam (resize flags); the Qt port keeps the two
// sliders and the histograms, and applies the change with the current world's
// creation params unchanged (the resize checkbox is exposed separately).
#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QSlider;

class ChangeTotalWorldHeightDialog : public QDialog
{
	Q_OBJECT
public:
	// minVx/maxVx: the loaded world's min/max voxel heights.
	// inputHistogram[256]: the sqrt-scaled column heights of the current world
	// (EngineViewport::worldHeightHistogram output).
	// initialHeight: the world's initialHeight (vrtMapCreationParam).
	ChangeTotalWorldHeightDialog(int minVx, int maxVx,
	                             const int inputHistogram[256],
	                             int initialHeight,
	                             QWidget* parent = nullptr);

	// Result values (read after exec() == Accepted):
	int deltaVx() const { return deltaVx_; }
	int scalePercent() const { return scalePercent_; }
	bool resizeWorld() const { return resizeWorld_; }

protected:
	// Draws both histograms; the input is static, the output is computed from
	// the current slider values.
	void paintEvent(QPaintEvent* event) override;

private slots:
	void onSliderChanged();
	void onOk();

private:
	// Recompute outHistogram_ from inputHistogram_ + current slider state
	// (inHistogram2OutHistogram port).
	void updateOutputHistogram();

	QLabel* inputMinLabel_ = nullptr;
	QLabel* inputMaxLabel_ = nullptr;
	QLabel* outMinLabel_ = nullptr;
	QLabel* outMaxLabel_ = nullptr;
	QSlider* deltaSlider_ = nullptr;
	QSlider* scaleSlider_ = nullptr;
	QLabel* deltaValueLabel_ = nullptr;
	QLabel* scaleValueLabel_ = nullptr;
	QCheckBox* resizeCheck_ = nullptr;

	// Histogram data. input_ comes from the world; out_ is recomputed.
	int inputHistogram_[256] = {0};
	int outHistogram_[256] = {0};
	int minVx_ = 0;
	int maxVx_ = 0;
	int initialHeight_ = 0;

	// Slider results.
	int deltaVx_ = 0;       // in voxels (<< VX_FRACTION applied on apply)
	int scalePercent_ = 100;

	bool resizeWorld_ = false;
};
