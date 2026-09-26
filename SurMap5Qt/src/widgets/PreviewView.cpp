// PreviewView.cpp — see header.

#include "PreviewView.h"

#include <QHideEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

PreviewView::PreviewView(QWidget* parent)
	: QWidget(parent)
{
	// The widget must own a native window for the engine's render window to
	// wrap (SDL_CreateWindowWithProperties from a foreign HWND).
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setMinimumSize(160, 120);

	// Animate: the effects preview needs continuous frames; the UI preview is
	// static but repainting is cheap and keeps both in sync.
	timer_ = new QTimer(this);
	timer_->setInterval(16);
	connect(timer_, &QTimer::timeout, this, [this]{ if(isVisible()) update(); });
}

void PreviewView::setPreviewFunctions(AttachFn attach, RenderFn render, DetachFn detach)
{
	attach_ = std::move(attach);
	render_ = std::move(render);
	detach_ = std::move(detach);
}

void PreviewView::ensureAttached()
{
	if(attached_ || !attach_)
		return;
	attached_ = attach_(reinterpret_cast<void*>(static_cast<quintptr>(winId())));
}

void PreviewView::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	ensureAttached();
	if(timer_)
		timer_->start();
}

void PreviewView::hideEvent(QHideEvent* event)
{
	if(timer_)
		timer_->stop();
	if(attached_ && detach_)
		detach_();
	attached_ = false;
	QWidget::hideEvent(event);
}

void PreviewView::paintEvent(QPaintEvent* /*event*/)
{
	ensureAttached();
	if(attached_ && render_){
		const qreal dpr = devicePixelRatioF();
		if(render_(qMax(1, (int)(width() * dpr)), qMax(1, (int)(height() * dpr))))
			return;
	}
	QPainter painter(this);
	painter.fillRect(rect(), QColor(24, 32, 40));
	painter.setPen(Qt::lightGray);
	painter.drawText(rect(), Qt::AlignCenter, tr("Preview unavailable"));
}

void PreviewView::mousePressEvent(QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton)
		lastMouse_ = event->position().toPoint();
	QWidget::mousePressEvent(event);
}

void PreviewView::mouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::LeftButton){
		const QPoint pos = event->position().toPoint();
		const QPoint delta = pos - lastMouse_;
		lastMouse_ = pos;
		emit orbited(delta.x() * 0.01f, delta.y() * 0.01f);
	}
	QWidget::mouseMoveEvent(event);
}

void PreviewView::wheelEvent(QWheelEvent* event)
{
	const int dy = event->angleDelta().y();
	if(dy != 0)
		emit zoomed(dy > 0 ? 0.9f : 1.1f);
	QWidget::wheelEvent(event);
}
