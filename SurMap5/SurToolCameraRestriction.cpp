#include "stdafx.h"
#include "SurMap5.h"

#include "SurMapOptions.h"
#include "SurToolCameraRestriction.h"
#include "SelectionUtil.h"

#include "..\UserInterface\UI_Render.h"
#include "..\UserInterface\UI_Minimap.h"
#include "Environment\Environment.h"

#include "Game\CameraManager.h"

#include "MainFrame.h"
#include "GeneralView.h"

IMPLEMENT_DYNAMIC(CSurToolCameraRestriction, CSurToolBase)

CSurToolCameraRestriction::CSurToolCameraRestriction(CWnd* parent)
: CSurToolBase(IDD, parent)
, selectedEdges_(0, 0, 0, 0)
, startPoint_(Vect3f::ZERO)
, layout_(this)
{

}

CSurToolCameraRestriction::~CSurToolCameraRestriction()
{
}

void CSurToolCameraRestriction::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_ZOOM_SLIDER, zoomSlider_);
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolCameraRestriction, CSurToolBase)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_HSCROLL()

	ON_COMMAND(IDC_TOP_LESS_BUTTON, OnTopLessButton)
	ON_COMMAND(IDC_TOP_MORE_BUTTON, OnTopMoreButton)

	ON_COMMAND(IDC_BOTTOM_LESS_BUTTON, OnBottomLessButton)
	ON_COMMAND(IDC_BOTTOM_MORE_BUTTON, OnBottomMoreButton)

	ON_COMMAND(IDC_LEFT_LESS_BUTTON, OnLeftLessButton)
	ON_COMMAND(IDC_LEFT_MORE_BUTTON, OnLeftMoreButton)

	ON_COMMAND(IDC_RIGHT_LESS_BUTTON, OnRightLessButton)
	ON_COMMAND(IDC_RIGHT_MORE_BUTTON, OnRightMoreButton)
END_MESSAGE_MAP()

BOOL CSurToolCameraRestriction::OnInitDialog()
{
	__super::OnInitDialog();

	LayoutVBox* vvbox = new LayoutVBox;
	vvbox->pack(new LayoutControl(IDC_ZOOM_SLIDER, this, 3));
	vvbox->pack(new LayoutControl(IDC_H_LINE, this, 1));

	LayoutHBox* hbox = new LayoutHBox;
	{
		LayoutVBox* vbox = new LayoutVBox;
		{
			vbox->pack(new LayoutElement(), true);
			{
				LayoutHBox* hbox = new LayoutHBox;
				hbox->pack(new LayoutControl(IDC_LEFT_LESS_BUTTON, this));
				hbox->pack(new LayoutControl(IDC_LEFT_MORE_BUTTON, this));
				vbox->pack(hbox);
			}
			vbox->pack(new LayoutElement(), true);
		}
		hbox->pack(vbox, false, false);
		
		vbox = new LayoutVBox;
		{
			vbox->pack(new LayoutControl(IDC_TOP_LESS_BUTTON, this));
			vbox->pack(new LayoutControl(IDC_TOP_MORE_BUTTON, this));
			vbox->pack(new LayoutElement(), true, true);
			vbox->pack(new LayoutControl(IDC_BOTTOM_LESS_BUTTON, this));
			vbox->pack(new LayoutControl(IDC_BOTTOM_MORE_BUTTON, this));
		}
		hbox->pack(vbox, true, false);

		vbox = new LayoutVBox;
		{
			vbox->pack(new LayoutElement(), true);
			{
				LayoutHBox* hbox = new LayoutHBox;
				hbox->pack(new LayoutControl(IDC_RIGHT_LESS_BUTTON, this));
				hbox->pack(new LayoutControl(IDC_RIGHT_MORE_BUTTON, this));
				vbox->pack(hbox);
			}
			vbox->pack(new LayoutElement(), true);
		}
		hbox->pack(vbox, false);
	}
	vvbox->pack(hbox, true, true);

	layout_.add(vvbox);

	zoomSlider_.SetRange(100, 200);
	zoomSlider_.SetPos(clamp(round(100.0f / miniMapZoom()), 100, 200));
	surMapOptions.setCameraBorderSelection(selectedEdges_);
	return TRUE;
}

void CSurToolCameraRestriction::OnSize(UINT type, int cx, int cy)
{
	if(::IsWindow(GetSafeHwnd()) && ::IsWindowVisible(GetSafeHwnd()))
		layout_.relayout();
}

void CSurToolCameraRestriction::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int pos = zoomSlider_.GetPos();
	float zoom = 1.0f / (float(pos) * 0.01f);
	setMiniMapZoom(zoom);	

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSurToolCameraRestriction::onLMBDown(const Vect2f& point)
{
	startPoint_ = point;
	lastPoint_ = point;

	Rectf rect(cameraBorder());

	float precision = 10.0f;

	selectedEdges_.set(0, 0, 0, 0);

	if(point.x < rect.left() + precision)
		selectedEdges_.left(1);
	if(point.y < rect.top() + precision)
		selectedEdges_.top(1);

	if(point.x > rect.right() - precision)
		selectedEdges_.width(1);
	if(point.y > rect.bottom() - precision)
		selectedEdges_.height(1);

	if(selectedEdges_ == Recti(0, 0, 0, 0))
		selectedEdges_.set(1, 1, 1, 1);

	surMapOptions.setCameraBorderSelection(selectedEdges_);
}

void CSurToolCameraRestriction::onLMBUp(const Vect2f& point)
{
	selectedEdges_.set(0, 0, 0, 0);
	surMapOptions.setCameraBorderSelection(selectedEdges_);
}

void CSurToolCameraRestriction::onMouseMove(const Vect2f& point)
{
	Vect2f delta(point - lastPoint_);
	move(delta);
    
	lastPoint_ = point;
}

bool CSurToolCameraRestriction::onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	Vect3f point = projectScreenPointOnPlane(Vect3f::K, Vect3f(0.0f, 0.0f, vMap.initialHeight), screenCoord);
	onLMBDown(point);
	return true;
}

bool CSurToolCameraRestriction::onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	Vect3f point = projectScreenPointOnPlane(Vect3f::K, Vect3f(0.0f, 0.0f, vMap.initialHeight), screenCoord);
	onLMBUp(point);
	return true;
}

bool CSurToolCameraRestriction::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoords)
{
	Vect3f point = projectScreenPointOnPlane(Vect3f::K, Vect3f(0.0f, 0.0f, vMap.initialHeight), screenCoords);

	onMouseMove(point);

	return true;
}

bool CSurToolCameraRestriction::onOperationOnMap(int x, int y)
{
    return true;
}

bool CSurToolCameraRestriction::onPreviewLMBDown(const Vect2f& point)
{
	onLMBDown(minimap().minimap2world(point));
	return true;
}

bool CSurToolCameraRestriction::onPreviewLMBUp(const Vect2f& point)
{
	onLMBUp(minimap().minimap2world(point));
	return true;
}

bool CSurToolCameraRestriction::onPreviewTrackingMouse(const Vect2f& point)
{
	onMouseMove(minimap().minimap2world(point));
	return true;
}

bool CSurToolCameraRestriction::onDrawPreview(int width, int height)
{
	Color4c color(surMapOptions.cameraBorderColor_);
	Color4c selectedColor(255, 255, 128);

	Recti rect = cameraBorder();

	Vect2f left_top = minimap().world2minimap(rect.left_top());
	Vect2f size = minimap().world2minimap(rect.right_bottom()) - left_top;

	UI_Render::instance().drawLine(left_top, left_top + Vect2f(0.0f, size.y), Color4f(selectedEdges_.left() ? selectedColor : color));
	UI_Render::instance().drawLine(left_top, left_top + Vect2f(size.x, 0.0f), Color4f(selectedEdges_.top() ? selectedColor : color));
	
	UI_Render::instance().drawLine(left_top + size, left_top + Vect2f(0.0f, size.y), Color4f(selectedEdges_.height() ? selectedColor : color));
	UI_Render::instance().drawLine(left_top + size, left_top + Vect2f(size.x, 0.0f), Color4f(selectedEdges_.width() ? selectedColor : color));
	return false;
}

bool CSurToolCameraRestriction::onDrawAuxData()
{
	if(!surMapOptions.showCameraBorders())
		mainFrame().view().drawCameraBorders();
	return true;
}

void CSurToolCameraRestriction::move(const Vect2f& delta)
{
	Rectf rect(cameraBorder());
	
	rect.left(rect.left() + float(selectedEdges_.left()) * delta.x);
	rect.top(rect.top() + float(selectedEdges_.top()) * delta.y);
	rect.width(rect.width() + float(selectedEdges_.width()) * delta.x - float(selectedEdges_.left()) * delta.x);
	rect.height(rect.height() + float(selectedEdges_.height()) * delta.y - float(selectedEdges_.top()) * delta.y);

	if(rect.width() < 0){
		rect.left(rect.right());
		rect.width(-rect.width());

		selectedEdges_.left(!selectedEdges_.left());
		selectedEdges_.width(!selectedEdges_.width());
	}
	if(rect.height() < 0){
		rect.top(rect.bottom());
		rect.height(-rect.height());

		selectedEdges_.left(!selectedEdges_.left());
		selectedEdges_.width(!selectedEdges_.width());
	}

	setCameraBorder(Recti(round(rect.left()), round(rect.top()), round(rect.width()), round(rect.height())));
}

Vect2f CSurToolCameraRestriction::buttonStep() const
{
	return Vect2f(64.0f, 64.0f);
}

Recti CSurToolCameraRestriction::cameraBorder()
{
	return cameraManager->cameraBorder().rect();
}

void CSurToolCameraRestriction::setCameraBorder(const Recti& rect)
{
	CameraBorder cameraBorder(cameraManager->cameraBorder());
	cameraBorder.setRect(rect);
	cameraManager->setCameraBorder(cameraBorder);
}

void CSurToolCameraRestriction::OnLeftLessButton()
{
	selectedEdges_.set(1, 0, 0, 0);
	move(-buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnLeftMoreButton()
{
	selectedEdges_.set(1, 0, 0, 0);
	move(buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnRightLessButton()
{
	selectedEdges_.set(0, 0, 1, 0);
	move(-buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnRightMoreButton()
{
	selectedEdges_.set(0, 0, 1, 0);
	move(buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnTopLessButton()
{
	selectedEdges_.set(0, 1, 0, 0);
	move(-buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnTopMoreButton()
{
	selectedEdges_.set(0, 1, 0, 0);
	move(buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnBottomLessButton()
{
	selectedEdges_.set(0, 0, 0, 1);
	move(-buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}

void CSurToolCameraRestriction::OnBottomMoreButton()
{
	selectedEdges_.set(0, 0, 0, 1);
	move(buttonStep());
	selectedEdges_.set(0, 0, 0, 0);
}


BOOL CSurToolCameraRestriction::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return __super::OnCommand(wParam, lParam);
}
