// RenderViewWidget.cpp — see header.

#include "RenderViewWidget.h"

#include <algorithm>

#include "editor/EngineViewport.h"

RenderViewWidget::RenderViewWidget(QWidget* parent)
	: QWidget(parent)
	, viewport_(new EngineViewport)
{
	// The engine's SDL GPU device claims a swapchain on a window created around
	// this widget's native handle (EngineViewport::init). It must therefore
	// always have one, and the engine draws straight into it.
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	setAttribute(Qt::WA_PaintOnScreen);
}

RenderViewWidget::~RenderViewWidget()
{
	doneRenderDevice();
	delete viewport_;
}

bool RenderViewWidget::initRenderDevice()
{
	if(viewport_->inited())
		return true;
	viewport_->setNativeWindow((void*)winId());
	return viewport_->init(std::max(1, width()), std::max(1, height()));
}

void RenderViewWidget::doneRenderDevice()
{
	viewport_->done();
}

void RenderViewWidget::tick()
{
	// Phase 3: CGeneralView::quant / CameraQuant / Animate go here, then the
	// viewport repaints — the MFC equivalent of view_->Invalidate(FALSE).
	update();
}

void RenderViewWidget::paintEvent(QPaintEvent* /*event*/)
{
	// Phase 2: the engine draws into this widget's swapchain. With no scene yet,
	// drawFrame presents the viewport's clear colour.
	if(!viewport_->inited())
		initRenderDevice();
	viewport_->drawFrame();
}

void RenderViewWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	viewport_->resize();
}
