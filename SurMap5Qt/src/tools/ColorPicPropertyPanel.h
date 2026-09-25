// ColorPicPropertyPanel.h — the Properties dock's panel for the texture brush.
// Qt port of CSurToolColorPic's dialog: the bitmap file picker, the centre-alpha
// and K/S/B sliders, the tint colour button and the "put to all world" button.
// Qt-side, talks to the engine-free ColorPicTool.

#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

class ColorPicTool;

class ColorPicPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ColorPicPropertyPanel(QWidget* parent = nullptr);

	void setTool(ColorPicTool* tool);

private:
	void browseTexture();
	void pickColor();
	void putToAllWorld();
	void refreshColorButton();

	QLineEdit* path_ = nullptr;
	QPushButton* browse_ = nullptr;
	QSlider* centerAlpha_ = nullptr;
	QSlider* kColor_ = nullptr;
	QSlider* saturation_ = nullptr;
	QSlider* brightness_ = nullptr;
	QPushButton* color_ = nullptr;
	QPushButton* putAll_ = nullptr;

	ColorPicTool* tool_ = nullptr;
};
