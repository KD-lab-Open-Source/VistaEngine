// GradientsPanel.h — Qt port of the gradient editor (CGradientsWindow,
// SurMap5/GradientsWindow.cpp) as a dock panel.
//
// The original was a CWnd hosting a stack of gradient strips (CGradientEditor
// View), a color selector and a ruler, driven by CKeyColor lists from the
// world's effects. The engine's gradient machinery (CKeyColor / NParticleKey)
// is not reachable from the Qt editor yet, so this panel is the editor UI
// shell: it knows how to draw a gradient strip and edit its key colors, and
// holds a small built-in palette so the control is usable standalone. The
// engine-backed gradient list (the world's gradients) lands with the effects
// data.

#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

// One draggable color stop of a gradient.
struct GradientKey
{
	float position = 0.f;   // 0..1 along the strip
	QColor color = Qt::white;
};

class GradientsPanel : public QWidget
{
	Q_OBJECT
public:
	explicit GradientsPanel(QWidget* parent = nullptr);

	// The gradient being edited (indices into the panel's list).
	int currentGradient() const { return current_; }
	void setCurrentGradient(int index);

signals:
	// The current gradient's keys changed (position/color moved).
	void gradientChanged(int index);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	// The built-in gradients (the engine list is deferred; the panel needs
	// something to edit). Two keys per gradient keeps each strip editable.
	QVector<QVector<GradientKey>> gradients_;
	int current_ = 0;

	// Drag state: which key of the current gradient is being dragged.
	int dragKey_ = -1;
};
