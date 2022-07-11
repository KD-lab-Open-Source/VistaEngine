#include "stdafx.h"

#include <algorithm>
#include <direct.h>

#include "MainFrame.h"
#include ".\EditorView.h"
#include "UIEditor.h"


#include "Options.h"
#include "SelectionCorner.h"
#include "SelectionManager.h"
#include "ActionManager.h"
#include "CreateControlAction.h"
#include "EraseAction.h"
#include "PositionChangeAction.h"
#include "PropertyChangeAction.h"
#include "ControlUtils.h"
#include "..\UserInterface\UI_Render.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\AttribEditor\AttribEditorCtrl.h"

#include "..\Environment\Environment.h" // только для drawBlackBars

#include "IVisGeneric.h"
#include "IVisD3D.h"
#include "IRenderDevice.h"
#include "..\Render\d3d\D3DRender.h"
#include "..\Game\RenderObjects.h"

#include "StreamCommand.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CUIEditorView

IMPLEMENT_DYNCREATE(CUIEditorView, CWnd)

const float    CUIEditorView::ASPECT                = 4.0f / 3.0f;
const float    CUIEditorView::CORNER_DISTANCE       = 0.0f;
const Vect2f   CUIEditorView::CORNER_SIZE           (6.0f, 6.0f); // px
const sColor4f CUIEditorView::ACTIVE_CORNER_COLOR   (1.0f, 1.0f, 0, 0.5f);
const sColor4f CUIEditorView::INACTIVE_CORNER_COLOR (0, 1.0f, 0, 0.25f);


BEGIN_MESSAGE_MAP(CUIEditorView, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
//	ON_WM_ERASEBKGND()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONUP()

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

ViewDefaultResources::ViewDefaultResources(cCamera* camera)
: reseted_(false)
, camera_(camera)
, screenDepth_(0)
, screenTexture_(0)
{
	initialize();
}

ViewDefaultResources::~ViewDefaultResources()
{
	free();
}

void ViewDefaultResources::free()
{
	RELEASE(screenTexture_);
	RELEASE(screenDepth_);
}

void ViewDefaultResources::DeleteDefaultResource()
{
	reseted_ = true;
	free();
}

void ViewDefaultResources::initialize()
{
	Vect2i res = Options::instance().resolution();
	if(!screenTexture_ || res.x != screenTexture_->GetWidth() || res.y != screenTexture_->GetHeight()){
		free();
		HRESULT hr = gb_RenderDevice3D->lpD3DDevice->CreateDepthStencilSurface(res.x, res.y, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, TRUE, &screenDepth_, 0);
		screenTexture_ = gb_VisGeneric->CreateRenderTexture(res.x, res.y, TEXTURE_RENDER32);
	}
	camera_->SetRenderTarget(screenTexture_, screenDepth_);
	UI_BackgroundScene::instance().setRenderTarget(screenTexture_, screenDepth_);
}

void ViewDefaultResources::RestoreDefaultResource()
{
	initialize();
	reseted_ = false;
}



CUIEditorView::CUIEditorView()
: current_screen_ (0)
, selecting_rect_ (false)
, selection_start_ (Vect2f::ZERO)
, selected_guide_ (-1)
, renderDeviceInitialized_(false)
, scrollStart_(Vect2f::ZERO)
, viewZoom_(1.0f)
, viewCenter_(0.5f, 0.5f)
{
    camera_ = 0;
    scene_ = 0;
}

CUIEditorView::~CUIEditorView()
{
    xassert(!camera_);
    xassert(!scene_);
}


void CUIEditorView::drawLine(const Vect2f& start, const Vect2f& end, const sColor4f& color)
{
	xassert(untransform(transform(start)).eq(start));
	xassert(untransform(transform(end)).eq(end));
	
	//UI_Render::instance().drawLine(screenCoords(start), screenCoords(end), color);
	Vect2i pos0(screenCoords(start));
	Vect2i pos1(screenCoords(end));
	gb_RenderDevice->DrawLine(pos0.x, pos0.y, pos1.x, pos1.y, color);
}

void CUIEditorView::drawRect(const Rectf& _rect, float _depth, const sColor4f& _color, eBlendMode _blend_mode, bool outline)
{
	xassert(untransform(transform(_rect)).eq(_rect));

	Rectf r(screenCoords(_rect));
	gb_RenderDevice->DrawRectangle(round(r.left()), round(r.top()),
								   round(r.width()),
								   round(r.height()), sColor4c(_color), outline);
	gb_RenderDevice->FlushPrimitive2D();
}

Rectf CUIEditorView::transform(const Rectf& coord)
{
	Vect2f leftTop = transform(coord.left_top());
	Vect2f rightBottom = transform(coord.right_bottom());
	return Rectf(leftTop, rightBottom - leftTop);
}

Vect2f CUIEditorView::transform(const Vect2f& coord)
{
	const Recti& windowPosition = UI_Render::instance().windowPosition();
	float centerX = float(gb_RenderDevice->GetSizeX()) / float(windowPosition.width()) * 0.5f;
	float centerY = float(gb_RenderDevice->GetSizeY()) / float(windowPosition.height()) * 0.5f;
	return (coord/* + leftTop*/ - viewCenter_) * Vect2f(viewZoom_, viewZoom_) + Vect2f(centerX, centerY);
}

Rectf CUIEditorView::untransform(const Rectf& coord)
{
	Vect2f leftTop = untransform(coord.left_top());
	Vect2f rightBottom = untransform(coord.right_bottom());
	return Rectf(leftTop, rightBottom - leftTop);
}

Vect2f CUIEditorView::untransform(const Vect2f& coord)
{
	const Recti& windowPosition = UI_Render::instance().windowPosition();
	float centerX = float(gb_RenderDevice->GetSizeX()) / float(windowPosition.width()) * 0.5f;
	float centerY = float(gb_RenderDevice->GetSizeY()) / float(windowPosition.height()) * 0.5f;
	return (coord - Vect2f(centerX, centerY)) * Vect2f(1.0f / viewZoom_, 1.0f / viewZoom_) + viewCenter_/* - leftTop*/;
}

void CUIEditorView::drawRectFrame(const Rectf& _rect, float _depth, const sColor4f& _color, const Vect2f& weight, eBlendMode _blend_mode)
{
	// left
    drawRect(Rectf(_rect.left() - weight.x, _rect.top(), weight.x, _rect.height() + weight.y), _depth, _color, _blend_mode);
	// right
    drawRect(Rectf(_rect.right(), _rect.top(), weight.x, _rect.height() + weight.y ), _depth, _color, _blend_mode);
	// top
    drawRect(Rectf(_rect.left() - weight.x, _rect.top() - weight.y, _rect.width() + 2.0f * weight.x, weight.y), _depth, _color, _blend_mode);
	// bottom
    drawRect(Rectf(_rect.left() - weight.x, _rect.bottom(), _rect.width() + 2.0f * weight.x, weight.y), _depth, _color, _blend_mode);
}

void CUIEditorView::redraw()
{
	static bool in_procedure = false;
	if (in_procedure)
		return;
	else
		in_procedure = true;

	if (gb_RenderDevice == 0 || gb_VisGeneric == 0 || !renderDeviceInitialized_) {
		in_procedure = false;
		return;
	}
	gb_RenderDevice->SelectRenderWindow(render_window_);

	streamLogicCommand.process(0);
	streamLogicCommand.clear();

	sColor4c workspaceColor(Options::instance().workspaceColor());
	gb_RenderDevice->Fill(workspaceColor.r, workspaceColor.g, workspaceColor.b);

	gb_RenderDevice->BeginScene();
	gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);

	camera_->SetFoneColor(Options::instance().backgroundColor());
	scene_->Draw(camera_);

	// т.к SetClipRect() для RenderTarget нормально не работает приходится вызывать SetViewport
	D3DVIEWPORT9 vp = { 0, 0, UI_Render::instance().camera()->GetRenderSize().x, UI_Render::instance().camera()->GetRenderSize().y, 0.0f, 1.0f };
	RDCALL(gb_RenderDevice3D->lpD3DDevice->SetViewport(&vp));

	gb_RenderDevice->SetFont(0);

	gb_RenderDevice->SetNoMaterial(ALPHA_NONE, MatXf::ID);
    
    gb_RenderDevice->SetRenderState(RS_CULLMODE, 1); // D3DCULL_NONE
	gb_RenderDevice->SetSamplerDataVirtual(0, sampler_wrap_linear);

	updateHoverControl();
	UI_Dispatcher::instance().redraw();

	gb_RenderDevice3D->RestoreRenderTarget();

	if(currentScreen())
		drawEditorUnderlay();
	afterRedraw();
	drawBlackBars();
	if(currentScreen())
		drawEditorOverlay();

	gb_RenderDevice->SetRenderState (RS_CULLMODE, -1);
	gb_RenderDevice->EndScene ();
	gb_RenderDevice->SelectRenderWindow (0);
	gb_RenderDevice->Flush ();

	in_procedure = false;
}

void CUIEditorView::quant(float dt)
{
	//if(currentScreen())
	//	currentScreen()->quant(dt);
	//UI_BackgroundScene::instance().graphQuant(dt);
	frame_time.next_frame();
	updateHoverControl();
	UI_Dispatcher::instance().logicQuant();
	UI_Dispatcher::instance().quant(dt);
}

void CUIEditorView::OnPaint()
{
	CPaintDC dc(this);
	
    redraw();
}

void CUIEditorView::finitRenderDevice()
{
	defaultResources_ = 0;
	UI_Render::instance().setCamera(0);
	UI_BackgroundScene::instance().done();
	UI_Dispatcher::instance().releaseResources();

	RELEASE(camera_);
    RELEASE(scene_);
	RELEASE(render_window_);

	RELEASE(gb_RenderDevice);
}

bool CUIEditorView::initRenderDevice(int _width, int _height)
{
	CreateIRenderDevice(false);
    if (!gb_RenderDevice->Initialize (200, 200, RENDERDEVICE_MODE_WINDOW, GetSafeHwnd ())) {
        xassert (0);
		return false;
    }
	
	render_window_ = gb_RenderDevice->CreateRenderWindow (GetSafeHwnd ());
	gb_RenderDevice->SelectRenderWindow (render_window_);

	UI_BackgroundScene::instance().init(gb_VisGeneric);
	UI_Render::instance().setInterfaces (gb_VisGeneric, gb_RenderDevice);

	scene_ = gb_VisGeneric->CreateScene ();
	camera_ = scene_->CreateCamera();
	defaultResources_ = new ViewDefaultResources(camera_);
		
    MatXf camera_matrix;
    Identity (camera_matrix);

    Vect3f camera_pos (0.0f, 0.0f, -100.0f);
    SetPosition(camera_matrix, camera_pos, Vect3f (0.0f, 0.0f, 0.0f));
	MatXf ml = MatXf::ID;
	ml.rot () [2][2] = -1;

	MatXf mr = MatXf::ID;
	mr.rot () [1][1] = -1;

    camera_->SetPosition (mr * ml * camera_matrix);

	UI_Render::instance().setCamera(camera_);

	gb_RenderDevice->SelectRenderWindow (0);
	return true;
}
void CUIEditorView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	view_width_ = cx;
	view_height_ = cy;

	if(gb_RenderDevice == 0)
        renderDeviceInitialized_ = false;
    
	if(renderDeviceInitialized_)
        gb_RenderDevice->ChangeSize(cx, cy, RENDERDEVICE_MODE_WINDOW);
	else
		renderDeviceInitialized_ = initRenderDevice(cx, cy);

	bool showAll = false;

	if(renderDeviceInitialized_){
		updateWindowPosition();
	}
}

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect);

void CUIEditorView::updateWindowPosition()
{
	CRect clientRect;
	GetClientRect(&clientRect);

	defaultResources_->initialize();
	Vect2i rulerSize(round(Options::instance().rulerWidth()), round(Options::instance().rulerWidth()));
	
	int cx = defaultResources_->screenTexture()->GetWidth();
	int cy = defaultResources_->screenTexture()->GetHeight();

	switch(Options::instance().zoomMode()){
	case Options::ZOOM_ONE_TO_ONE:
		setViewZoom(1.0f);
		break;
	case Options::ZOOM_SHOW_ALL:
		float zoom = min(1.0f, min(float(clientRect.Width()) / float(cx), float(clientRect.Height()) / float(cy)));
		setViewZoom(zoom);
		setViewCenter(Vect2f(0.5f, 0.5f));
		Options::instance().setZoomMode(Options::ZOOM_SHOW_ALL);
		break;
	}
	
    updateCameraFrustum(cx, cy);

	Vect2i res(Options::instance().resolution());
	

	Recti rect(aspectedWorkArea(Rectf(0.0f, 0.0f, res.x, res.y), 4.0f / 3.0f));
	UI_Render::instance().setWindowPosition(rect);
}

void CUIEditorView::updateCameraFrustum(int _width, int _height)
{
	gb_RenderDevice->SelectRenderWindow (render_window_);
    //float corrected_height = float(_height) / float(_width) * 4.0f / 3.0f;

    camera_->SetFrustum(&Vect2f(0.5f, 0.5f),
                        &sRectangle4f(-0.5f, -0.5f,
                                       0.5f,  0.5f), 
                        &Vect2f(1.0f, 1.0f),
                        &Vect2f(1.0f, 3000.0f)
                        );
	gb_RenderDevice->SelectRenderWindow (0);
}


void CUIEditorView::drawHandle(const Vect2f& pos, const Vect2f& corner_size, bool active)
{
	sColor4f color = active ? CUIEditorView::ACTIVE_CORNER_COLOR : CUIEditorView::INACTIVE_CORNER_COLOR;
	
    Rectf rect(pos.x - corner_size.x , pos.y - corner_size.y,
			   corner_size.x * 2.0f, corner_size.y * 2.0f);
	drawRect(rect, 0.0f, color, ALPHA_ADDBLEND);
	//UI_Render::instance().drawSprite(rect, 0, color, UI_BLEND_APPEND);
}

struct ControlAuxDrawer {
	Vect2f cursor_position_;
	Vect2f corner_size_;
	float corner_distance_;
	CUIEditorView* view_;

    void drawCorners (const Rectf& rect) {
        SelectionCorner::Corners::const_iterator it;
        CornerUnderPoint is_corner_under_control (cursor_position_, rect, corner_size_, corner_distance_);
        FOR_EACH (SelectionCorner::allCorners (), it) {
            Vect2f pos = it->getCornerPosition (rect, corner_distance_);
			view_->drawHandle(pos, corner_size_, is_corner_under_control(*it));
        }
    }

	ControlAuxDrawer(CUIEditorView* view, const Vect2f& cursor_position, const Vect2f& corner_size, float corner_distance)
	: cursor_position_ (cursor_position)
	, corner_size_ (corner_size)
	, corner_distance_ (corner_distance)
	, view_(view)
	{}

	void operator()(const UI_ControlBase* control){
		xassert(control);
		operator()(*control);
	}
	void operator()(const UI_ControlBase& control){
		view_->drawRect(control.position (), -5.0f,
				  CUIEditorView::ACTIVE_CORNER_COLOR, ALPHA_ADDBLEND);
		
		if(view_->isEditingSubRect()){
			Rectf controlRect = control.position();

			int rectsCount = control.getSubRectCount ();
			for (int i = 0; i < rectsCount; ++i) {
				Rectf subRect = control.getSubRect (i);
				Rectf rect (controlRect.width()  * subRect.left() + controlRect.left(),
							controlRect.height() * subRect.top()  + controlRect.top(),
							controlRect.width()  * subRect.width(),
							controlRect.height() * subRect.height());
				
				view_->drawRectFrame (rect, -6.0f, sColor4f (float(((i + 1) >> 2) % 2) * 0.5f + 0.5f,
													  float(((i + 1) >> 1) % 2) * 0.5f + 0.5f,
													  float(((i + 1) >> 0) % 2) * 0.5f + 0.5f, 200),
							   Vect2f (2.0f * view_->pixelWidth(), 2.0f * view_->pixelHeight()),
							   ALPHA_ADDBLEND);
                drawCorners (rect);
			}
		}
		if(view_->isEditingMask()){
			UI_Mask::Polygon::const_iterator it;
			const UI_Mask::Polygon& polygon = control.mask().polygon();
			if(!polygon.empty()){
				sColor4f lineColor(1.0f, 1.0f, 1.0f, 1.0f);
				Vect2f point1(Vect2f::ZERO), point2(Vect2f::ZERO);
				for(it = polygon.begin(); it != polygon.end(); ){
					point1 = *it;
					if(++it != polygon.end())
						point2 = *it;
					else
						point2 = polygon.front();

					Vect2f offset(control.transfPosition().left_top());

					point1 = point1 + offset;
					point2 = point2 + offset;
					
					view_->drawHandle(point1, corner_size_, Rectf(point1 - corner_size_, corner_size_ * 2.0f).point_inside(cursor_position_));

					view_->drawLine(point1, point2, lineColor);
				}
			}
		}
	}
};

float CUIEditorView::pixelWidth() const
{
	return 1.0f / (float(UI_Render::instance().windowPosition().width())/* - 2.0f * Options::instance().rulerWidth()*/) / viewZoom_;
}

float CUIEditorView::pixelHeight() const
{
	return 1.0f / (float(UI_Render::instance().windowPosition().height())/* - 2.0f * Options::instance().rulerWidth ()*/) / viewZoom_;
}


void CUIEditorView::drawEditorUnderlay(const UI_ControlContainer* container)
{
	if(!container){
		container = currentScreen();
		if(!container)
			return;
	}

	UI_ControlContainer::ControlList::const_iterator it;
	FOR_EACH(container->controlList(), it){
		const UI_ControlBase* control = *it;
		Rectf position = control->position();
		if(isControlVisibleInEditor(*control) && !Rectf(0.0f, 0.0f, 1.0f, 1.0f).rect_inside(position))
			drawRect(control->position(), 0.0f, sColor4f(0.0f, 0.0f, 0.0f, 1.0f), ALPHA_BLEND, true);
		drawEditorUnderlay(control);
	}
}

void CUIEditorView::drawEditorOverlay()
{
	Selection& selection = SelectionManager::the ().selection ();
	ControlAuxDrawer drawer(this, cursor_position_, CORNER_SIZE * Vect2f(pixelWidth(), pixelHeight()), CORNER_DISTANCE);
	if (!selection.empty() && isEditingControls())
        drawer.drawCorners(selection.calculateBounds ());
	Selection::iterator it;
	FOR_EACH (selection, it) {
		drawer (*it);
	}

	Rectf rect(selection_start_, cursor_position_ - selection_start_	);
	rect.validate ();
	if(selecting_rect_) {
		drawRectFrame (rect, -6.0f, sColor4f (0, 255, 0, 200), 
						Vect2f (2.0f * pixelWidth(),
								2.0f * pixelHeight()),
						ALPHA_ADDBLEND);
	}


	// сетка
	int ix = 0, iy = 0;
	Vect2i largeGrid = Options::instance().largeGridSize();
	if (Options::instance().showGrid ()) {
		Vect2f grid_size = Options::instance().calculateRelativeGridSize();
		for (float x = 0.0f; x <= 1.0f; x += grid_size.x) {
			Rectf rect (x, 0.0f, pixelWidth(), 1.0f);
			//if(drawSmallGrid || !ix)
			drawRect (rect, -6.0f, sColor4f (1.0f, 1.0f, 1.0f, 0.15f + float(!ix) * 0.15f), ALPHA_BLEND);
			ix = (ix + 1) % largeGrid.x;
		}
		for (float y = 0.0f; y <= 1.0f; y += grid_size.y) {
			Rectf rect (0.0f, y, 1.0f, pixelHeight());
			//if(drawSmallGrid || !iy)
			drawRect (rect, -6.0f, sColor4f (1.0f, 1.0f, 1.0f, 0.15f + float(!iy) * 0.15f), ALPHA_BLEND);
			iy = (iy + 1) % largeGrid.y;
		}
	}

	// края экрана
	if (Options::instance().showBorder()) {
		sColor4f color(1.0f, 1.0f, 1.0f, 0.5f);
		Rectf rect(0.0f, 0.0f, 1.0f, 1.0f);
		drawRectFrame(rect, -6.0f, color, Vect2f(pixelWidth(), pixelHeight()), ALPHA_BLEND);
	}
	return;
}

void addControlsFromContainer (UI_ControlContainer& container,
							   UI_ControlContainer::ControlList& dest) {
	const UI_ControlContainer::ControlList& controls = container.controlList ();
	UI_ControlContainer::ControlList::const_iterator it;
	UI_ControlContainer::ControlList::iterator sub_it;
	FOR_EACH (controls, it) {
		for (sub_it = dest.begin();; ++sub_it) {
			if (sub_it == dest.end() || (*it)->screenZ () > (*sub_it)->screenZ()) {
				dest.insert (sub_it, *it);
				break;
			}
		}
		addControlsFromContainer (**it, dest);
	}
}

void CUIEditorView::OnMButtonDown(UINT nFlags, CPoint point)
{
	scrollStart_.set(point.x, point.y);
	scrollStartRelative_ = relativeCoords(scrollStart_);
	SetFocus();
}

void CUIEditorView::OnMButtonUp(UINT nFlags, CPoint point)
{
}

Vect2f CUIEditorView::relativeCoords(const Vect2f& screenCoords)
{
	Vect2f windowSize(UI_Render::instance().windowPosition().size());
	return untransform(Vect2f(screenCoords) * Vect2f(1.0f / windowSize.x, 1.0f / windowSize.y));
}

Rectf CUIEditorView::screenCoords(const Rectf& relativeCoords)
{
	Vect2f left_top(screenCoords(relativeCoords.left_top()));
	Vect2f right_bottom(screenCoords(relativeCoords.right_bottom()));

	return Rectf(left_top, right_bottom - left_top);
}

Vect2f CUIEditorView::screenCoords(const Vect2f& relativeCoords)
{
	Vect2f windowSize(UI_Render::instance().windowPosition().size());
	return transform(relativeCoords) * windowSize;
	//return UI_Render::instance().screenCoords(relativeCoords);
}

void CUIEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();
	Vect2f screen_point = relativeCoords(Vect2i(point.x, point.y));
	
	if (currentScreen() == 0)
		return;

	if(isEditingSubRect()){
		UI_ControlBase* control = SelectionManager::the().selection().front();
		sub_rect_editor_.startDrag(screen_point);
		ActionManager::the().setChanged();
	}
    else if(isEditingMask()){
		UI_ControlBase* control = SelectionManager::the().selection().front();
		mask_editor_.setMask(&control->mask(), control->transfPosition());
        mask_editor_.startDrag(screen_point);
		ActionManager::the().setChanged();
    }
	else if(isEditingControls()){
		if (!SelectionManager::the().selection().empty ()) {
			if (!SelectionManager::the().startDrag(screen_point)) {
				// code moved into OnRButtonDown
			}
		}
	}
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CUIEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();
	Vect2f screen_point = relativeCoords(Vect2i(point.x, point.y));
	
	if(currentScreen() == 0)
		return;

	if(isEditingControls()){
		UI_ControlContainer::ControlList::reverse_iterator it;
		UI_ControlContainer::ControlList controls;
		addControlsFromContainer (*currentScreen(), controls);

		it = std::find_if (controls.rbegin (), controls.rend (), ControlUnderPoint (screen_point));
		if(it != controls.rend()){
			UI_ControlBase* control = *it;
			if (control) {
				if (nFlags & MK_SHIFT) {
					SelectionManager::the ().toggleSelection (control);
					updateProperties();
				} else {
					if (!SelectionManager::the().isSelected(control)) {
						SelectionManager::the().select(control);
						onSelectionChanged ();
						updateProperties();
					}
					return CWnd::OnLButtonDown(nFlags, point);
				}
			}
		}
		if(!SelectionManager::the().selection().empty()) {
			if (!(nFlags & MK_SHIFT)) {
				SelectionManager::the ().deselectAll ();
				onSelectionChanged ();
				updateProperties();
			}
		}
	}
	else if(isEditingMask()){
		if(mask_editor_.startRDrag(cursor_position_))
			updateProperties();
	}
	else if(isEditingSubRect()){
	}
	else{
		selection_start_ = cursor_position_;
		selecting_rect_ = true;
	}
	CWnd::OnRButtonDown(nFlags, point);
}

void CUIEditorView::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (selecting_rect_) {
		if (!(nFlags & MK_SHIFT)) {
			SelectionManager::the().deselectAll ();
		}
		Rectf rect (selection_start_, cursor_position_ - selection_start_);
		rect.validate ();

		UI_ControlContainer::ControlList::iterator it;
		UI_ControlContainer::ControlList controls;
		addControlsFromContainer (*currentScreen(), controls);

		FOR_EACH(controls, it){
			UI_ControlBase* control = *it;
			if(::isControlVisibleInEditor(*control) && control->position().rect_overlap(rect)) {
				if(!SelectionManager::the().isSelected(control)) {
					SelectionManager::the().toggleSelection(control);
				}
			}
		}
		selecting_rect_ = false;
		onSelectionChanged();
		updateProperties();
	}
	CWnd::OnRButtonUp(nFlags, point);
}


void CUIEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
	cursor_position_ = relativeCoords(Vect2i(point.x, point.y));

	if(nFlags & MK_MBUTTON){
		Vect2f scrollDelta = (relativeCoords(Vect2i(point.x, point.y)) - scrollStartRelative_);
		setViewCenter(viewCenter() - scrollDelta);
		updateWindowPosition();
	}
	scrollStart_.set(point.x, point.y);		
	scrollStartRelative_ = relativeCoords(scrollStart_);

	if(selected_guide_ >= 0){
		Options::Guides::iterator it;
		it = Options::instance().guides().begin();
		std::advance(it, selected_guide_);
		if (it->type == Options::Guide::HORIZONTAL)
			it->position = cursor_position_.y;
		else
			it->position = cursor_position_.x;
	}
    else{
		bool preserveAspect = nFlags & MK_SHIFT;
		SelectionManager::the().drag(cursor_position_, preserveAspect);
		sub_rect_editor_.drag(cursor_position_, preserveAspect);
		mask_editor_.drag(cursor_position_);
	}
	CWnd::OnMouseMove(nFlags, point);
}

void CUIEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (selected_guide_ >= 0) {
		Options::Guides::iterator it = Options::instance().guides().begin();
		std::advance(it, selected_guide_);
		if (it->position < 0.0f || it->position > 1.0f) {
			Options::instance().guides().erase (it);
		}
		selected_guide_ = -1;
	}

	if(SelectionManager::the().endDrag())
		updateProperties();

	if(sub_rect_editor_.endDrag())
		updateProperties();
	
	if(mask_editor_.endDrag())
		updateProperties();
	
	CWnd::OnLButtonUp(nFlags, point);
}

void CUIEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CUIMainFrame* mainFrame = ((CUIMainFrame*)AfxGetMainWnd());

	Vect2i resolution(UI_Render::instance().windowPosition().size());

	Vect2f delta = Vect2f::ZERO;
	switch (nChar){
	case VK_LEFT:
		delta.x = -1.0f / resolution.x * static_cast<float>(nRepCnt);
		break;
	case VK_RIGHT:
		delta.x = 1.0f / resolution.x * static_cast<float>(nRepCnt);
		break;
	case VK_UP:
		delta.y = -1.0f / resolution.y * static_cast<float>(nRepCnt);
		break;
	case VK_DOWN:
		delta.y = 1.0f / resolution.y * static_cast<float>(nRepCnt);
		break;
	case VK_DELETE:
		mainFrame->OnEditDelete();
		break;
	}
	ActionManager::the().act(new PositionChangeAction (SelectionManager::the().selection(), delta));
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CUIEditorView::updateHoverControl()
{
	if(!SelectionManager::the().selection().empty())
		UI_LogicDispatcher::instance().setHoverControl(SelectionManager::the().selection().front());
	else
		UI_LogicDispatcher::instance().setHoverControl(0);
}

void CUIEditorView::updateProperties()
{
	Selection& selection = SelectionManager::the().selection ();
	CAttribEditorCtrl* attrib_editor = CUIEditorApp::GetMainFrame()->GetAttribEditor();
	if (!currentScreen())
		return;

	ASSERT(attrib_editor);
	if(selection.size() == 0)
		attrib_editor->attachSerializeable(Serializeable(*currentScreen()));
	else if(selection.size () == 1)
		attrib_editor->attachSerializeable(Serializeable(*selection.front()));
	else
		attrib_editor->detachData();
}


void CUIEditorView::OnDestroy()
{
	finitRenderDevice();
	CWnd::OnDestroy();
}

void CUIEditorView::eraseSelection()
{
	Selection& selection = SelectionManager::the().selection();

    if(selection.empty())
        return;
	
	CAttribEditorCtrl& attribEditor = *CUIEditorApp::GetMainFrame()->GetAttribEditor();

    attribEditor.detachData();
	ActionManager::the().act(new EraseControlAction(selection, *currentScreen()));
}


void CUIEditorView::setCurrentScreen(UI_Screen* screen)
{
	if(screen && current_screen_ != screen)
		UI_Dispatcher::instance().selectScreen(screen);//screen->activate();

    current_screen_ = screen;
	SelectionManager::the ().deselectAll ();
}


void CUIEditorView::onSelectionChanged ()
{
	Selection& selection = SelectionManager::the().selection();
	if (selection.size() == 1) {
		CUIEditorApp::GetMainFrame ()->GetControlsTree ()->selectControlInScreen (selection.front(), current_screen_);
	} else if (selection.empty()) {
		CUIEditorApp::GetMainFrame ()->GetControlsTree ()->selectScreen (current_screen_);
	}
}

bool CUIEditorView::isEditingControls() const
{
	return (!GetAsyncKeyState(VK_CONTROL) && !GetAsyncKeyState(VK_MENU));
}

bool CUIEditorView::isEditingSubRect() const
{
	return GetAsyncKeyState(VK_CONTROL) && !GetAsyncKeyState(VK_SHIFT) && SelectionManager::the().selection().size() == 1;
}

bool CUIEditorView::isEditingMask() const
{
	return GetAsyncKeyState(VK_MENU) && !GetAsyncKeyState(VK_SHIFT) && SelectionManager::the().selection().size() == 1;
}

void CUIEditorView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar == VK_MENU)
		return;

	CWnd::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

void CUIEditorView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar == VK_MENU)
		return;

	CWnd::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CUIEditorView::OnEraseBkgnd(CDC* pDC)
{
    return FALSE;
	//return CWnd::OnEraseBkgnd(pDC);
}

BOOL CUIEditorView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	cursor_position_ = relativeCoords(Vect2i(point.x, point.y));
	Options::instance().setZoomMode(Options::ZOOM_CUSTOM);
	if(nFlags & MK_LBUTTON){
		scrollStart_ = Vect2i(point.x, point.y);
		scrollStartRelative_ = cursor_position_;
	}
	zoom(zDelta > 0);
	
	updateWindowPosition();
	return CWnd::OnMouseWheel(nFlags, zDelta, point);
}


static const float ZOOM_TABLE[] = { 0.05f, 0.10f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
static const int ZOOM_TABLE_SIZE = sizeof(ZOOM_TABLE) / sizeof(ZOOM_TABLE[0]);

void CUIEditorView::zoom(bool in)
{
	if(in){
		for(int i = 0; i < ZOOM_TABLE_SIZE; ++i){
			float zoom = ZOOM_TABLE[i];
			if(fabs(zoom - viewZoom()) < FLT_COMPARE_TOLERANCE)
				continue;
			if(viewZoom() < zoom){
				setViewZoom(zoom);
				return;
			}
		}
		setViewZoom(ZOOM_TABLE[ZOOM_TABLE_SIZE - 1]);
	}
	else{
		for(int i = ZOOM_TABLE_SIZE - 1; i >= 0; --i){
			float zoom = ZOOM_TABLE[i];
			if(fabs(zoom - viewZoom()) < FLT_COMPARE_TOLERANCE)
				continue;
			if(viewZoom() > zoom){
				setViewZoom(zoom);
				return;
			}
		}
		setViewZoom(ZOOM_TABLE[0]);
	}


}

void CUIEditorView::setViewZoom(float zoom)
{
	viewZoom_ = zoom;
	if(fabs(zoom - 1.0f) < FLT_COMPARE_TOLERANCE)
		Options::instance().setZoomMode(Options::ZOOM_ONE_TO_ONE);
	else
		Options::instance().setZoomMode(Options::ZOOM_CUSTOM);

	Vect2f cornerSize = CORNER_SIZE * Vect2f(pixelWidth(), pixelHeight());
	SelectionManager::the().setCornerSize(cornerSize);
	sub_rect_editor_.setCornerSize(cornerSize);
	mask_editor_.setPixelSize(Vect2f(pixelWidth(), pixelHeight()));
}

float CUIEditorView::viewZoom() const
{
	return viewZoom_;
}

void CUIEditorView::setViewCenter(const Vect2f& viewCenter)
{
	viewCenter_ = viewCenter;
}

const Vect2f& CUIEditorView::viewCenter() const
{
	return viewCenter_;
}

void CUIEditorView::afterRedraw()
{
	if(!defaultResources_->reseted()){
		Vect2f zoom(viewZoom_, viewZoom_);
		Vect2i res = Options::instance().resolution();
		const Recti& windowPosition = UI_Render::instance().windowPosition();
		
		//float relativeWidth = float(gb_RenderDevice->GetSizeX()) / float(res.x);
		//float relativeHeight = float(gb_RenderDevice->GetSizeY()) / float(res.y);

		float relativeWidth = 1.0f;
		float relativeHeight = 1.0f;
		
		//Rectf viewRect = Rectf(0.0f, 0.0f, float(gb_RenderDevice->GetSizeX()) / float(windowPosition.width()), float(gb_RenderDevice->GetSizeY()) / float(windowPosition.height()));
		
		Rectf entireScreen	= screenCoords(UI_Render::instance().entireScreenCoords());
		Rectf viewRect(0.0f, 0.0f, float(gb_RenderDevice->GetSizeX()), float(gb_RenderDevice->GetSizeY()));

		Rectf intersected	= viewRect.intersection(entireScreen);
		Rectf uv			= intersected / entireScreen;

	
		if(viewZoom_ + FLT_COMPARE_TOLERANCE < 1.0f)
			gb_RenderDevice->SetSamplerDataVirtual(0, sampler_wrap_linear);
		else
			gb_RenderDevice->SetSamplerDataVirtual(0, sampler_wrap_point);

		gb_RenderDevice->SetClipRect(0, 0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
		
		gb_RenderDevice->DrawSprite(round(intersected.left()), round(intersected.top()), round(intersected.width()), round(intersected.height()),
									uv.left(), uv.top(), uv.width(), uv.height(),
									defaultResources_->screenTexture(), sColor4c(255, 255, 255, 255), 0.0f, ALPHA_NONE);
	}
}


void CUIEditorView::drawBlackBars()
{
	const Vect2i& renderSize = UI_Render::instance().renderSize();
	const Recti& windowPosition = UI_Render::instance().windowPosition();
	Rectf entireScreen = UI_Render::instance().entireScreenCoords();
	if(renderSize.y == windowPosition.height())
		return;
	sColor4f color(0.0f, 0.0f, 0.0f, 1.0f);

	float top = windowPosition.top() / float(windowPosition.height());
	float bottom = (renderSize.y - windowPosition.bottom()) / float(windowPosition.height());
	drawRect(Rectf(0.0f, -top, entireScreen.width(), top), 0.0f, color);
	drawRect(Rectf(0.0f, 1.0f, entireScreen.width(), bottom), 0.0f, color);
}
