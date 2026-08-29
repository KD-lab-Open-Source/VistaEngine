// MiniMapPanel.cpp — see header.

#include "MiniMapPanel.h"

#include <vector>

#include <QMouseEvent>
#include <QPainter>

#include "RenderViewWidget.h"

MiniMapPanel::MiniMapPanel(RenderViewWidget* view, QWidget* parent)
	: QWidget(parent)
	, view_(view)
{
	// The minimap widget itself is small and never wants keyboard focus; it is
	// an overlay, not an interactive 3D surface.
	setFocusPolicy(Qt::NoFocus);
	setMinimumSize(64, 64);
}

void MiniMapPanel::reload()
{
	// The engine's minimap pixel size (vMap.H_SIZE/16 x V_SIZE/16) and the
	// world size (vMap.H_SIZE x V_SIZE) — the panel scales the image up and
	// maps clicks back to world units with them.
	int sizex = 0, sizey = 0;
	if(!view_->minimapSize(sizex, sizey)){
		image_ = QImage();
		worldW_ = 0;
		worldH_ = 0;
		update();
		return;
	}

	// Read the minimap pixels through the engine-free facade (the
	// vMap::saveMiniMap port, Terra/VMAP.CPP:917).
	std::vector<unsigned long> pixels((size_t)sizex * (size_t)sizey);
	if(!view_->minimapPixels(pixels.data(), sizex, sizey)){
		image_ = QImage();
		worldW_ = 0;
		worldH_ = 0;
		update();
		return;
	}

	QImage image((const uchar*)pixels.data(), sizex, sizey, sizex * 4,
	             QImage::Format_ARGB32);
	image_ = image.copy();   // deep copy: `pixels` dies at scope end
	worldW_ = sizex * 16;
	worldH_ = sizey * 16;
	update();
}

bool MiniMapPanel::worldSize(int& hSize, int& vSize) const
{
	if(worldW_ <= 0 || worldH_ <= 0)
		return false;
	hSize = worldW_;
	vSize = worldH_;
	return true;
}

void MiniMapPanel::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	const QRect r = rect();

	if(image_.isNull()){
		painter.fillRect(r, QColor(32, 32, 32));
		painter.setPen(Qt::gray);
		painter.drawText(r, Qt::AlignCenter, tr("No world"));
		return;
	}

	// Draw the map scaled to fit the widget, keeping the aspect ratio — the
	// same "letterbox" the original's updateMinimapPosition computed with the
	// widget aspect and the map aspect (MiniMapWindow.cpp:255).
	const float aspect = (float)worldW_ / (float)worldH_;
	QRect target = r;
	if((float)r.width() / (float)r.height() > aspect){
		const int w = (int)(r.height() * aspect);
		target.setLeft(r.left() + (r.width() - w) / 2);
		target.setRight(target.left() + w);
	}
	else{
		const int h = (int)(r.width() / aspect);
		target.setTop(r.top() + (r.height() - h) / 2);
		target.setBottom(target.top() + h);
	}
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.drawImage(target, image_);

	// The camera marker: the orbit centre in world units, mapped through the
	// same fit (the original drew the view-zone rectangle over the map).
	float cx = 0.f, cy = 0.f;
	if(view_->cameraCenter(cx, cy)){
		const QPointF p(
			target.left() + (cx / worldW_) * target.width(),
			target.top() + (cy / worldH_) * target.height());
		painter.setPen(QPen(QColor(0, 255, 0, 220), 2));
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(p, 8.0, 8.0);
		painter.drawLine(p + QPointF(-11, 0), p + QPointF(11, 0));
		painter.drawLine(p + QPointF(0, -11), p + QPointF(0, 11));
	}
}

void MiniMapPanel::mousePressEvent(QMouseEvent* event)
{
	// A minimap click: convert the widget point to world coordinates and move
	// the camera there (CMiniMapWindow::OnLButtonDown -> pressEvent ->
	// cameraToEvent).
	if(event->button() == Qt::LeftButton && view_->worldLoaded()){
		const QRect r = rect();
		const float aspect = (float)worldW_ / (float)worldH_;
		QRect target = r;
		if((float)r.width() / (float)r.height() > aspect){
			const int w = (int)(r.height() * aspect);
			target.setLeft(r.left() + (r.width() - w) / 2);
			target.setRight(target.left() + w);
		}
		else{
			const int h = (int)(r.width() / aspect);
			target.setTop(r.top() + (r.height() - h) / 2);
			target.setBottom(target.top() + h);
		}
		if(target.width() > 0 && target.height() > 0){
			const QPointF p = event->position();
			const float wx = (p.x() - target.left()) / (float)target.width() * worldW_;
			const float wy = (p.y() - target.top()) / (float)target.height() * worldH_;
			view_->setCameraCenter(wx, wy);
			update();
		}
	}
	event->accept();
}
