// UITextureEditor.cpp : implementation file
//
#include "stdafx.h"

#include "my_STL.h"
#include <vector>
using namespace std;

#include "UISpriteEditor.h"
#include "IRenderDevice.h"
#include "IVisGeneric.h"
#include "Rect.h"
#include "UISpriteEditorDlg.h"
#include "UISpriteEditor.h"
#include ".\uispriteeditor.h"

BEGIN_MESSAGE_MAP(CUISpriteEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

const float ZoomTable [] = { 16.0f, 8.0f, 4.0f, 3.0f, 
					         2.0f, 1.0f, 0.5f, 0.25f, 0.1f };
enum { MaxZoom = 9 };

// CUISpriteEditor
IMPLEMENT_DYNAMIC(CUISpriteEditor, CWnd)
CUISpriteEditor::CUISpriteEditor()
: view_offset_(Vect2f::ZERO)
, scrolling_(false)
, moving_(false)
, zoom_(5)

, backgroundColor_(255, 0, 255, 255)

, animationPhase_(0.0f)
, animationTime_(0.0f)
, animationEnabled_(true)

, renderWindow_(0)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, UITEXTUREEDITOR_CLASSNAME, &wndclass) )
	{
		wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= UITEXTUREEDITOR_CLASSNAME;

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

CUISpriteEditor::~CUISpriteEditor()
{
}


static const float ScreenWidth = 1.0f;
static const float ScreenHeight = 1.0f;
static const float Aspect = 4.0f / 3.0f;

void CUISpriteEditor::drawRectTextured (const Rectf& _rect, float _depth, const sColor4c& _color, cTexture* _texture, const Rectf& _uv_rect, eBlendMode _blend_mode, float phase, float saturation)
{
	Rectf rect = coordsToWindow(_rect);
    gb_RenderDevice->DrawSprite(round(rect.left()), round(rect.top()), round(rect.width()), round(rect.height()),
                                _uv_rect.left(), _uv_rect.top(), _uv_rect.width(), _uv_rect.height(), _texture, _color, phase, _blend_mode, saturation);
}

void CUISpriteEditor::drawRect (const Rectf& _rect, float _depth, const sColor4c& _color, eBlendMode _blend_mode)
{
    drawRectTextured(_rect, _depth, _color, 0, Rectf ());
}


void CUISpriteEditor::clipRectByView(Rectf* rect, Rectf* uv)
{
	xassert(rect);
	Rectf view = coordsFromWindow(Rectf(0.0f, 0.0f, viewSize().x, viewSize().y));
	Rectf inter = view.intersection(*rect);
	if(uv){
		Rectf r = *rect;
		Rectf scale = (inter  - r.left_top()) / (r - r.left_top());
		*uv = *uv * scale;
	}
	*rect = inter;
}

void CUISpriteEditor::redraw()
{
	if(gb_RenderDevice->IsInBeginEndScene() || !renderWindow_)
		return;

	float zoom = ZoomTable[zoom_];

	gb_RenderDevice->SelectRenderWindow(renderWindow_);
	gb_RenderDevice->Fill(backgroundColor_.r, backgroundColor_.g, backgroundColor_.b);
    gb_RenderDevice->BeginScene();
	gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);
	scene_->Draw(camera_);
	gb_RenderDevice->SetFont(0);

	gb_RenderDevice->SetNoMaterial(ALPHA_NONE, MatXf::ID);
    
    gb_RenderDevice->SetRenderState(RS_CULLMODE, 1/*= D3DCULL_NONE*/);
	gb_RenderDevice->SetSamplerDataVirtual(0, zoom < 1.0f ? sampler_wrap_linear : sampler_wrap_point);

	if(!sprite_.isEmpty()){
		if(cTexture* texture = sprite_.texture()){

			float width = texture->GetWidth();
			float height = texture->GetHeight();

			Rectf textureRect = Rectf(0.0f, 0.0f, width, height);
			Rectf uv(0.0f, 0.0f, 1.0f, 1.0f);
			clipRectByView(&textureRect, &uv);

			drawRectTextured(textureRect, 5.0f, sprite_.diffuseColor(), texture, uv, ALPHA_BLEND, animationPhase_, sprite_.saturation());
			Rectf pixeledCoords =  sprite_.textureCoords() * Vect2f(width, height);

			drawEdgesOverlay(pixeledCoords);
		}
	}

    gb_RenderDevice->EndScene ();
    gb_RenderDevice->Flush ();
	gb_RenderDevice->SelectRenderWindow (0);
}

void CUISpriteEditor::OnPaint()
{
	CPaintDC dc(this);
    redraw();
}

void CUISpriteEditor::finitRenderDevice()
{
    RELEASE(scene_);
	RELEASE(camera_);
	RELEASE(renderWindow_);
}

void CUISpriteEditor::initRenderDevice(int width, int height)
{
	renderWindow_ = gb_RenderDevice->CreateRenderWindow (GetSafeHwnd ());
	gb_RenderDevice->SelectRenderWindow(renderWindow_);

	scene_ = gb_VisGeneric->CreateScene();
	camera_ = scene_->CreateCamera();

    MatXf camera_matrix;
    Identity(camera_matrix);

    Vect3f camera_pos(0.0f, 0.0f, -100.0f);
    SetPosition(camera_matrix, camera_pos, Vect3f(0.0f, 0.0f, 0.0f));
    
	MatXf ml = MatXf::ID;
	ml.rot()[2][2] = -1;

	MatXf mr = MatXf::ID;
	mr.rot()[1][1] = -1;

    //camera_->SetPosition(mr * ml * camera_matrix);
	OnSize(0, width, height);
	gb_RenderDevice->SelectRenderWindow(0);
}

void CUISpriteEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	view_size_.x = cx;
	view_size_.y = cy;

	if(gb_RenderDevice && renderWindow_){
		renderWindow_->ChangeSize();
	    updateCameraFrustum(cx, cy);
	}
	
}

void CUISpriteEditor::updateCameraFrustum(int _width, int _height)
{
    gb_RenderDevice->SelectRenderWindow(renderWindow_);
    float corrected_height = 0.75f;

    float aspect = 4.0f / 3.0f;
    float width = 1.0f;
    float height = 0.75f;

    float px_width = static_cast<float>(_width);

    camera_->SetFrustum (&Vect2f (0.5f, 0.5f),
                           &sRectangle4f (-0.5f * width, -0.5f * height,
										   0.5f * width,  0.5f * height), 
                           &Vect2f (1.0f / px_width, 1.0f / aspect / px_width),
                           &Vect2f (1.0f, 3000.0f)
                          );

    gb_RenderDevice->SelectRenderWindow(0);
}

void CUISpriteEditor::OnDestroy()
{
	finitRenderDevice();
	CWnd::OnDestroy();
}

struct MoveHelper{
	MoveHelper(Vect2f& offset)
	: offset_(offset)
	{

	}

	void start (const Vect2f& point){
	}
	void move (const Vect2f& point){
	}
	void stop (const Vect2f& point){
	}
	Vect2f& offset_;
};

void CUISpriteEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();
	scrolling_ = true;
	SetCapture ();
	
	CWnd::OnRButtonDown(nFlags, point);
}

void CUISpriteEditor::scroll (const Vect2f& delta)
{
    view_offset_ += delta;
}

void CUISpriteEditor::move (const Vect2f& pos, const Vect2i& delta)
{
    Rectf coords = getCoords ();

	if (selected_edges_.left () &&
		selected_edges_.top () &&
		selected_edges_.width () &&
		selected_edges_.height ())
	{
		coords = coords + Vect2f (delta.x, delta.y);
	}
	else {
		if (selected_edges_.left ()) {
			coords.left(coords.left() + delta.x);
			coords.width(coords.width () - delta.x);
		}
		if (selected_edges_.width ()) {
			coords.width(coords.width() + delta.x);
		}
		if (selected_edges_.top ()) {
			coords.top(coords.top() + delta.y);
			coords.height(coords.height() - delta.y);
		}
		if (selected_edges_.height ()) {
			coords.height(coords.height() + delta.y);
		}
	}
	if(coords.width() < 0){
		coords.left(coords.right());
		coords.width(-coords.width());
		selected_edges_.left(!selected_edges_.left());
		selected_edges_.width(!selected_edges_.width());
	}
	if(coords.height() < 0){
		coords.top(coords.bottom());
		coords.height(-coords.height());
		selected_edges_.top(!selected_edges_.top());
		selected_edges_.height(!selected_edges_.height());
	}
    setCoords(coords);
	static_cast<CUISpriteEditorDlg*>(GetParent ())->UpdateControls ();
}

void CUISpriteEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	static Vect2f deltaDiff (Vect2f::ZERO);
	static CPoint lastPoint = point;
	CPoint deltaPoint = point - lastPoint;

    Vect2f delta = Vect2f (deltaPoint.x, deltaPoint.y) / ZoomTable [zoom_];
	Vect2f deltaRounded (round (delta.x + deltaDiff.x), round (delta.y + deltaDiff.y));
	deltaDiff += delta - deltaRounded;
    if(scrolling_){
        scroll(delta);
		redraw();
    }
    if(moving_){
        move(coordsFromWindow (Vect2f (point.x, point.y)), deltaRounded);
		redraw();
    }
	lastPoint = point;
	CWnd::OnMouseMove(nFlags, point);
}

void CUISpriteEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	scrolling_ = false;
	if (this == GetCapture ()) {
		ReleaseCapture ();
	} else {
		CWnd::OnRButtonUp(nFlags, point);
	}
}

BOOL CUISpriteEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	zoom_ = max (0, min (MaxZoom - 1, zoom_ + -zDelta / abs(zDelta)));
	CRect rt;
	GetClientRect(&rt);
	updateCameraFrustum(rt.Width(), rt.Height());
	redraw();
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

Rectf CUISpriteEditor::coordsToWindow (const Rectf& rect)
{
    return Rectf(coordsToWindow (Vect2f (rect.left(), rect.top())),
                 rect.size () * ZoomTable[zoom_]);
}

Vect2f CUISpriteEditor::coordsFromWindow (const Vect2f& point)
{
    return (point - Vect2f(viewSize()) * 0.5f) / ZoomTable [zoom_] - viewOffset();
}

Rectf CUISpriteEditor::coordsFromWindow(const Rectf& rect)
{
	Vect2f left_top(coordsFromWindow(rect.left_top()));
	Vect2f right_bottom(coordsFromWindow(rect.right_bottom()));
    return Rectf(left_top, right_bottom - left_top);
}

Vect2f CUISpriteEditor::coordsToWindow (const Vect2f& point)
{
    return (point + viewOffset()) * ZoomTable[zoom_] + Vect2f(viewSize()) * 0.5f;
}

void CUISpriteEditor::drawEdgesOverlay(const Rectf& rect)
{
    sColor4c guideColor (0, 0, 255, 128);
    sColor4c selectedColor (255, 255, 0, 200);
    float zOffset = 12.0f;

	Vect2f pixelSize = Vect2f(3.0f / ZoomTable[zoom_], 3.0f / ZoomTable[zoom_]) ;

	Rectf view_rect(coordsFromWindow(Rectf(0.0f, 0.0f, viewSize().x, viewSize().y)));



	// mask
	sColor4c maskColor(255.0f, 0.0f, 0.0f, 64.0f);
	eBlendMode maskMode = ALPHA_ADDBLEND;
	zOffset = 11.0f;

	drawRect(Rectf(view_rect.left(), view_rect.top(), rect.left() - view_rect.left(), view_rect.height()), zOffset, maskColor, maskMode);
	drawRect(Rectf(rect.left(), view_rect.top(), rect.width (), rect.top() - view_rect.top()), zOffset, maskColor, maskMode);

	drawRect(Rectf(rect.left(), rect.bottom(), rect.width(), view_rect.bottom() - rect.bottom ()), zOffset, maskColor, maskMode);
	drawRect(Rectf(rect.right(), view_rect.top(), view_rect.right() - rect.right(), view_rect.height()), zOffset, maskColor, maskMode);

	// edges
	drawRect(Rectf(rect.left() - pixelSize.x, view_rect.top(), pixelSize.x, view_rect.height()),         zOffset, selected_edges_.left() ? selectedColor : guideColor);
	drawRect(Rectf(rect.right(), view_rect.top(), pixelSize.x, view_rect.height()),  					 zOffset, selected_edges_.width() ? selectedColor : guideColor);

    drawRect(Rectf(view_rect.left(), rect.top() - pixelSize.y, view_rect.width(), pixelSize.y),			 zOffset, selected_edges_.top() ? selectedColor : guideColor);
	drawRect(Rectf(view_rect.left(), rect.bottom(), view_rect.width(), pixelSize.y), 					 zOffset, selected_edges_.height() ? selectedColor : guideColor);
}

void CUISpriteEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
    //point -= CPoint (viewSize ().x / 2, viewSize().y / 2);
	SetFocus ();
	if (!sprite_.texture ())
		return;

	Rectf rect = coordsToWindow (sprite_.textureCoords () * Vect2f (sprite_.texture ()->GetWidth (),
                                                                     sprite_.texture ()->GetHeight ()));
	if (nFlags & MK_CONTROL) {
		Vect2f mousePoint = coordsFromWindow (Vect2f(point.x, point.y));
		Rectf rect = getCoords();
		rect.left(mousePoint.x);
		rect.top(mousePoint.y);
		rect.width(0.0f);
		rect.height(0.0f);
		setCoords(rect);
		//move (mousePoint, mousePoint - Vect2f (sprite_.textureCoords ().left (), sprite_.textureCoords ().top ()));
		selected_edges_ = Recti (0, 0, 1, 1);
		//move (mousePoint, mousePoint - Vect2f (sprite_.textureCoords ().right (), sprite_.textureCoords ().bottom ()));
		moving_ = true;
	} else {
		selected_edges_ = Recti (0, 0, 0, 0);
		int precision = 1 + round (ZoomTable [zoom_]);
		if(round(rect.left()) > point.x - precision){
			selected_edges_.left (1);
			moving_ = true;
		}
		if(round(rect.right()) < point.x + precision){
			selected_edges_.width (1);
			moving_ = true;
		}
		if(round(rect.top()) > point.y - precision){
			selected_edges_.top (1);
			moving_ = true;
		}
		if(round(rect.bottom()) < point.y + precision){
			selected_edges_.height (1);
			moving_ = true;
		}
		if(!moving_){
			if (rect.point_inside (Vect2f (point.x, point.y))) {
				selected_edges_ = Recti (1, 1, 1, 1);
				moving_ = true;
			}
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CUISpriteEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	moving_ = false;
	selected_edges_ = Recti (0, 0, 0, 0);
	redraw();
	CWnd::OnLButtonUp(nFlags, point);
}

void CUISpriteEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	Vect2f delta (Vect2f::ZERO);
	if (nChar == VK_LEFT) {
		delta.set (-1.0f, 0.0f);
	} else if (nChar == VK_RIGHT) {
		delta.set (1.0f, 0.0f);
	} else if (nChar == VK_UP) {
		delta.set (0.0f, -1.0f);
	} else if (nChar == VK_DOWN) {
		delta.set (0.0f, 1.0f);
	} else {
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	setCoords(getCoords () + delta * static_cast<float>(nRepCnt));
	redraw();
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CUISpriteEditor::setSprite( const UI_Sprite& sprite )
{
	sprite_ = sprite;
}

UI_Sprite CUISpriteEditor::getSprite() const
{
	return sprite_;
}

void CUISpriteEditor::setTexture(const char* name)
{
	//UI_Texture* texture = const_cast<UI_Texture*>(UI_TextureLibrary::instance ().find (name));
	if (name) {
		UI_TextureReference texture (name);
		sprite_.setTextureReference (texture);
	} else {
		UI_TextureReference texture;
		sprite_.setTextureReference (texture);
	}
	redraw ();
}

int CUISpriteEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetTimer(101, 10, 0);

	return 0;
}

void CUISpriteEditor::nextFrame()
{
	if(sprite_.isAnimated()){
		cTexture* texture = sprite_.texture();
		animationTime_ += 0.001f * float(texture->GetTotalTime()) / float(texture->GetNumberFrame());
	}
}

void CUISpriteEditor::prevFrame()
{
	if(sprite_.isAnimated()){
		cTexture* texture = sprite_.texture();
		animationTime_ -= 0.001f * float(texture->GetTotalTime()) / float(texture->GetNumberFrame());
	}
}

void CUISpriteEditor::enableAnimation(bool enable)
{
	animationEnabled_ = enable;
}

void CUISpriteEditor::OnTimer(UINT nIDEvent)
{
	static double lastTime = xclock() * 0.001;
	double currentTime = xclock() * 0.001;
	float delta = min(0.1f, float(currentTime - lastTime));
	
	if(animationEnabled_)
		animationTime_ += delta;

	animationPhase_ = sprite_.phase(animationTime_, true);
	redraw();
	lastTime = currentTime;
	CWnd::OnTimer(nIDEvent);
}

void CUISpriteEditor::PreSubclassWindow()
{
	SetTimer(101, 10, 0);

	CWnd::PreSubclassWindow();
}

void CUISpriteEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if(!sprite_.isEmpty() && sprite_.texture())
		setCoords(Rectf(0, 0, sprite_.texture()->GetWidth(), sprite_.texture()->GetHeight()));
	CWnd::OnLButtonDblClk(nFlags, point);
}
