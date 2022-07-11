#include "StdAfx.h"
#include "NoisePreview.h"
#include "kdw/_WidgetWindow.h"
#include "kdw/Win32/MemoryDC.h"
#include "kdw/Win32/Handle.h"

#include "Render/src/NParticle.h"

class NoisePreviewImpl : public kdw::_WidgetWindow{
public:
	const char* className() const{ return "kdw.NoisePreivew"; }
	NoisePreviewImpl(NoisePreview* owner);

	void redraw(HDC dc);
	void update(bool refresh);

	void onMessagePaint();
	BOOL onMessageEraseBkgnd(HDC dc);
	void onMessageLButtonDown(UINT button, int x, int y);
	void set(const NoiseParams& params);
protected:
	NoisePreview* owner_;
	NoiseParams params_;
	std::vector<float> points_;
	PerlinNoise noise_;
};

NoisePreviewImpl::NoisePreviewImpl(NoisePreview* owner)
: kdw::_WidgetWindow(owner)
, owner_(owner)
{
	create("", WS_CHILD, Recti(0, 0, 40, 40), *Win32::_globalDummyWindow, WS_EX_CLIENTEDGE);
	update(true);
}

void NoisePreviewImpl::update(bool refresh)
{
	points_.clear();
	RECT rect;
	GetClientRect(*this, &rect);
	points_.resize(rect.right - rect.left);
	int numOctaves = params_.octaveAmplitudes.size();
	noise_.SetParameters(16, params_.frequency, params_.amplitude, params_.octaveAmplitudes, params_.onlyPositive, refresh);
	int size = points_.size();
	for (int i = 0; i < size; i++)
		points_[i] = noise_.Get(float(i + 1)/(float(size)/2));
	RedrawWindow(*this, 0, 0, RDW_INVALIDATE);
}

void NoisePreviewImpl::onMessageLButtonDown(UINT button, int x, int y)
{
	update(true);
}

BOOL NoisePreviewImpl::onMessageEraseBkgnd(HDC dc)
{
	return FALSE;
}

void NoisePreviewImpl::set(const NoiseParams& params)
{
	bool refresh = params.octaveAmplitudes.size() != params_.octaveAmplitudes.size()
		|| params.onlyPositive != params_.onlyPositive;
	params_ = params;
	update(refresh);
}

void NoisePreviewImpl::redraw(HDC dc)
{
	RECT rect;
	GetClientRect(*this, &rect);
	Win32::Handle<HRGN> rgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
	SelectClipRgn(dc, rgn);
	SelectObject(dc, GetStockObject(WHITE_BRUSH));
	Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
	MoveToEx(dc, rect.left, (rect.bottom-rect.top) / 2, 0);
	LineTo(dc, rect.right, (rect.bottom-rect.top) / 2);

	MoveToEx(dc, (rect.right-rect.left)/2,rect.top, 0);
	LineTo(dc, (rect.right-rect.left)/2,rect.bottom);

	// Draw Noise
	Win32::Handle<HPEN> pen(CreatePen(PS_SOLID,1,RGB(255,0,0)));
	HGDIOBJ oldPen = SelectObject(dc, &pen);

	for(int i=0; i < points_.size(); i++){
		int y = round(points_[i] * 50.0f);
		if (i == 0)
			MoveToEx(dc, i, (rect.bottom-rect.top) / 2 - y, 0);
		else
			LineTo(dc, i, (rect.bottom-rect.top) / 2 - y);
	}
	SelectObject(dc, oldPen);
}

void NoisePreviewImpl::onMessagePaint()
{
	PAINTSTRUCT paintStruct;
	HDC dc = BeginPaint(*this, &paintStruct);
	{
		Win32::MemoryDC memoryDC(dc);
		redraw(memoryDC);
	}
	EndPaint(*this, &paintStruct);
}

// --------------------------------------------------------------------------- 

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list

NoisePreview::NoisePreview(int border)
: kdw::_WidgetWithWindow(new NoisePreviewImpl(this), border)
{
}

#pragma warning(pop)

void NoisePreview::set(const NoiseParams& params)
{
	impl().set(params);
}

NoisePreviewImpl& NoisePreview::impl()
{
	return static_cast<NoisePreviewImpl&>(*__super::_window());
}
