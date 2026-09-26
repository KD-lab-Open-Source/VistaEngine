// PreviewView.cpp — see header.

#include "PreviewView.h"

#include <QHideEvent>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>

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
