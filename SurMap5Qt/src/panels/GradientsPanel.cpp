// GradientsPanel.cpp — see header.

#include "GradientsPanel.h"

#include <algorithm>

#include <QMouseEvent>
#include <QPainter>

namespace {
const int kStripHeight = 18;   // one gradient strip's height in pixels
const int kKeySize = 10;       // the draggable key marker's half-size
}

GradientsPanel::GradientsPanel(QWidget* parent)
	: QWidget(parent)
{
	// A small built-in set so the panel is usable without the engine's
	// gradient list. The world's real gradients (CKeyColor lists) replace
	// these once the effects data is wired in.
	QVector<GradientKey> day, sunset, night;
	day.append({0.00f, QColor(0x7f, 0xbf, 0xff)});
	day.append({0.50f, QColor(0xff, 0xff, 0xff)});
	day.append({1.00f, QColor(0xff, 0xc8, 0x60)});
	sunset.append({0.00f, QColor(0x2a, 0x2a, 0x8a)});
	sunset.append({0.50f, QColor(0xff, 0x7f, 0x40)});
	sunset.append({1.00f, QColor(0x40, 0x10, 0x50)});
	night.append({0.00f, QColor(0x08, 0x08, 0x28)});
	night.append({0.50f, QColor(0x30, 0x38, 0x68)});
	night.append({1.00f, QColor(0x08, 0x08, 0x28)});
	gradients_ = { day, sunset, night };

	setMinimumHeight(kStripHeight + 24);
	setFocusPolicy(Qt::NoFocus);
}

void GradientsPanel::setCurrentGradient(int index)
{
	if(index < 0 || index >= (int)gradients_.size())
		return;
	current_ = index;
	update();
}

void GradientsPanel::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	const QRect r = rect();

	painter.fillRect(r, palette().window());

	// One horizontal strip per gradient, top to bottom (the original's
	// gradientsBox_ packed CGradientEditorViews vertically).
	const int stripW = r.width() - 16;
	for(int g = 0; g < (int)gradients_.size(); ++g){
		const int top = 8 + g * (kStripHeight + 16);
		const QRect strip(r.left() + 8, top, stripW, kStripHeight);

		// The strip itself: a horizontal gradient between the keys.
		QLinearGradient grad(strip.left(), 0, strip.right(), 0);
		for(const GradientKey& key : gradients_[g])
			grad.setColorAt(key.position, key.color);
		painter.fillRect(strip, grad);
		painter.setPen(Qt::darkGray);
		painter.drawRect(strip);

		// The draggable key markers (the original's CGradientPositionCtrl
		// markers over the strip).
		for(const GradientKey& key : gradients_[g]){
			const int x = strip.left() + (int)(key.position * strip.width());
			const QRect mark(x - kKeySize / 2, strip.top() - kKeySize / 2,
			                 kKeySize, kKeySize);
			painter.setPen(Qt::black);
			painter.setBrush(key.color);
			painter.drawRect(mark);
		}
	}

	// Which gradient is being edited (currentGradient_ label).
	painter.setPen(palette().text().color());
	painter.drawText(r.left() + 8, r.bottom() - 4,
	                 tr("Gradient %1 / %2").arg(current_ + 1).arg(gradients_.size()));
}

int keyAt(const QVector<GradientKey>& keys, const QRect& strip, const QPoint& p)
{
	for(int i = 0; i < (int)keys.size(); ++i){
		const int x = strip.left() + (int)(keys[i].position * strip.width());
		const QRect mark(x - kKeySize / 2, strip.top() - kKeySize / 2,
		                 kKeySize, kKeySize);
		if(mark.contains(p))
			return i;
	}
	return -1;
}

void GradientsPanel::mousePressEvent(QMouseEvent* event)
{
	if(event->button() != Qt::LeftButton)
		return;

	const QRect r = rect();
	const int stripW = r.width() - 16;
	for(int g = 0; g < (int)gradients_.size(); ++g){
		const int top = 8 + g * (kStripHeight + 16);
		const QRect strip(r.left() + 8, top, stripW, kStripHeight);
		if(!strip.adjusted(-kKeySize, -kKeySize, kKeySize, kKeySize).contains(event->pos()))
			continue;

		current_ = g;
		const int hit = keyAt(gradients_[g], strip, event->pos());
		if(hit >= 0){
			dragKey_ = hit;
		}
		else{
			// Clicking an empty spot inserts a key of the strip's color at the
			// click position (sample the gradient there).
			QLinearGradient grad(strip.left(), 0, strip.right(), 0);
			for(const GradientKey& key : gradients_[g])
				grad.setColorAt(key.position, key.color);
			GradientKey key;
			key.position = (float)(event->pos().x() - strip.left()) / strip.width();
			key.color = grad.stops().isEmpty() ? QColor(Qt::white) : grad.stops().last().second;
			for(const QGradientStop& stop : grad.stops()){
				if(stop.first <= key.position)
					key.color = stop.second;
			}
			gradients_[g].append(key);
			std::sort(gradients_[g].begin(), gradients_[g].end(),
			          [](const GradientKey& a, const GradientKey& b){ return a.position < b.position; });
			dragKey_ = keyAt(gradients_[g], strip, event->pos());
		}
		update();
		emit gradientChanged(current_);
		break;
	}
	event->accept();
}

void GradientsPanel::mouseMoveEvent(QMouseEvent* event)
{
	if(dragKey_ < 0)
		return;

	const QRect r = rect();
	const int stripW = r.width() - 16;
	const int g = current_;
	const int top = 8 + g * (kStripHeight + 16);
	const QRect strip(r.left() + 8, top, stripW, kStripHeight);
	if(strip.width() <= 0)
		return;

	// Clamp the dragged key to the strip and keep it between its neighbours
	// (the original's CGradientPositionCtrl did the same).
	QVector<GradientKey>& keys = gradients_[g];
	float pos = (float)(event->pos().x() - strip.left()) / strip.width();
	pos = std::max(0.0f, std::min(1.0f, pos));
	if(dragKey_ > 0 && dragKey_ < keys.size() - 1){
		pos = std::max(pos, keys[dragKey_ - 1].position + 0.01f);
		pos = std::min(pos, keys[dragKey_ + 1].position - 0.01f);
	}
	keys[dragKey_].position = pos;
	update();
	emit gradientChanged(current_);
	event->accept();
}

void GradientsPanel::mouseReleaseEvent(QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton)
		dragKey_ = -1;
	event->accept();
}
