#include "StdAfx.h"
#include "UISpriteEditorViewport.h"
#include "Serialization/RangedWrapper.h"
#include "Serialization/Decorators.h"
#include "Render\Inc\IRenderDevice.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\Texture.h"
#include "Render\Src\Scene.h"
#include "Render\Src\VisGeneric.h"
#include "kdw/ContentUtil.h"
#include "kdw/Timer.h"
#include "FileUtils/FileUtils.h"
#include "Serialization/StringTableImpl.h"
#include "Serialization/SerializationFactory.h"
#include "UserInterface/UserInterface.h"
#include "kdw/Win32/Window.h"

const float ZoomTable [] = { 16.0f, 8.0f, 4.0f, 3.0f, 
					         2.0f, 1.0f, 0.5f, 0.25f, 0.1f };
enum { MaxZoom = 9 };

#pragma warning(push)
#pragma warning(disable: 4355) // warning C4355: 'this' : used in base member initializer list
UISpriteEditorViewport::UISpriteEditorViewport()
: view_offset_(Vect2f::ZERO)
, scrolling_(false)
, moving_(false)
, zoom_(5)
, animationPhase_(0.0f)
, animationTime_(0.0f)
, animationEnabled_(true)
, timer_(new kdw::Timer(this, 0.01f))
, kdw::Viewport(gb_RenderDevice)
{
	timer_->signalTimer().connect(this, &UISpriteEditorViewport::onTimer);
	fillColor_.set(128, 128, 128);
}
#pragma warning(pop)

UISpriteEditorViewport::~UISpriteEditorViewport()
{
	timer_ = 0;
}


void UISpriteEditorViewport::serialize(Archive& ar)
{
	UI_TextureReference textureReference = sprite_.textureReference();
	UI_TextureReference oldTexture = textureReference;
	ar.serialize(textureReference, "texture", "<Текстура");
	if(ar.isInput())
		sprite_.setTextureReference(textureReference);

	const char* filter[] = { TRANSLATE("Текстуры (tga, dds, avi)"), "*.tga;*.dds;*.avi", 0 };

	ButtonDecorator addButton(TRANSLATE("Добавить текстуру..."));
	ar.serialize(addButton, "addButton", "<");
	if(addButton){
		std::string filename = selectAndCopyResource("Resource\\UI\\Textures", filter, "", addButton.text, this);
		if(!filename.empty()){
			std::string fileTitle = ::extractFileName(filename.c_str());
			
			UI_Texture* texture = new UI_Texture(filename.c_str());
			texture->addRef();
			UI_TextureLibrary::instance().add(UI_TextureLibrary::StringType(fileTitle.c_str(), texture));
			sprite_.setTextureReference(UI_TextureReference(fileTitle.c_str()));
			UI_Dispatcher::instance().init();
		}
	}

	ButtonDecorator addLocButton(TRANSLATE("Добавить лок. текстуру..."));
	ar.serialize(addLocButton, "addLocButton", "<");
	if(addLocButton){
		string locPath = getLocDataPath("UI_Textures");
		std::string filename = selectAndCopyResource(locPath.c_str(), filter, "", addLocButton.text, this);
		if(!filename.empty()){
			std::string fileTitle = ::extractFileName(filename.c_str());

			UI_Texture* texture = new UI_Texture(filename.c_str());
			texture->addRef();
			UI_TextureLibrary::instance().add(UI_TextureLibrary::StringType(fileTitle.c_str(), texture));
			sprite_.setTextureReference(UI_TextureReference(fileTitle.c_str()));
			UI_Dispatcher::instance().init();
		}
	}

	ButtonDecorator deleteButton(TRANSLATE("Удалить текстуру"));
	ar.serialize(deleteButton, "deleteButton", "<");
	if(deleteButton){
		if(sprite_.textureReference()){
			if(::MessageBox(*_window(), TRANSLATE("Желаете удалить текстуру?"), TRANSLATE("Вопрос"), MB_YESNO | MB_ICONQUESTION) == IDYES){
				UI_TextureLibrary::instance().remove(sprite_.textureReference().c_str());
				UI_TextureLibrary::instance().buildComboList();
				UI_Dispatcher::instance().init();
				sprite_.setTextureReference(UI_TextureReference());
			}
		}
	}

	ar.serialize(HLineDecorator(), "hline1", "<");

	Vect2f textureSize = sprite_.texture() ? Vect2f(sprite_.texture()->GetWidth(), sprite_.texture()->GetHeight()) : Vect2f(1, 1);
	Rectf coords = sprite_.textureCoords() * textureSize;
	Vect2f leftTop = coords.left_top();
	Vect2f size = coords.size();
	ar.serialize(leftTop, "coords", "Координаты");
	ar.serialize(size, "size", "Размер");
	if(ar.isInput()){
		coords.set(leftTop, size);
		coords = coords / textureSize;
		sprite_.setTextureCoords(coords);
	}

	ButtonDecorator allButton(TRANSLATE("Выбрать всю текстуру"));
	ar.serialize(allButton, "allButton", "<");
	if(allButton){
		if(sprite_.texture())
			setCoords(Rectf(0, 0, sprite_.texture()->GetWidth(), sprite_.texture()->GetHeight()));
		else
			setCoords(Rectf(0, 0, 256.0f, 256.0f));
	}	

	ar.serialize(HLineDecorator(), "hline2", "<");

	Color4c diffuseColor = sprite_.diffuseColor();
	ar.serialize(diffuseColor, "diffuseColor", "Диффузный цвет");
	if(ar.isInput()){
		sprite_.setDiffuseColor(diffuseColor);
		sprite_.setTextureReference(sprite_.textureReference());
	}

	float saturation = sprite_.saturation();
	ar.serialize(RangedWrapperf(saturation, 0.0f, 1.0f, 0.01f), "saturation", "Насыщенность");
	if(ar.isInput())
		sprite_.setSaturation(saturation);

	ar.serialize(fillColor_, "backgroundColor", "Цвет фона");

	if(ar.isInput()){
		redraw();
		if(oldTexture != textureReference)
			signalTextureChanged().emit();
	}
}

static const float ScreenWidth = 1.0f;
static const float ScreenHeight = 1.0f;
static const float Aspect = 4.0f / 3.0f;

Rectf UISpriteEditorViewport::getCoords() const
{
	if (!sprite_.isEmpty () && sprite_.texture()) {
		const Rectf& coords = sprite_.textureCoords ();
		return Rectf(round (coords.left ()   * sprite_.texture ()->GetWidth ()),
					 round (coords.top ()    * sprite_.texture ()->GetHeight ()),
					 round (coords.width ()  * sprite_.texture ()->GetWidth ()),
					 round (coords.height () * sprite_.texture ()->GetHeight ()));
	}
	return Rectf ();
}
void UISpriteEditorViewport::setCoords(const Rectf& coords)
{
	if (!sprite_.isEmpty () && sprite_.texture()) {
		sprite_.setTextureCoords (coords / Vect2f (sprite_.texture ()->GetWidth (), sprite_.texture ()->GetHeight ()));
		redraw();
	}
}

void UISpriteEditorViewport::drawRectTextured (const Rectf& _rect, float _depth, const Color4c& _color, cTexture* _texture, const Rectf& _uv_rect, eBlendMode _blend_mode, float phase, float saturation)
{
	Rectf rect = coordsToWindow(_rect);
    gb_RenderDevice->DrawSprite(round(rect.left()), round(rect.top()), round(rect.width()), round(rect.height()),
                                _uv_rect.left(), _uv_rect.top(), _uv_rect.width(), _uv_rect.height(), _texture, _color, phase, _blend_mode, saturation);
}

void UISpriteEditorViewport::drawRect (const Rectf& _rect, float _depth, const Color4c& _color, eBlendMode _blend_mode)
{
    drawRectTextured(_rect, _depth, _color, 0, Rectf ());
}


void UISpriteEditorViewport::clipRectByView(Rectf* rect, Rectf* uv)
{
	xassert(rect);
	Rectf view = coordsFromWindow(Rectf(0.0f, 0.0f, size().x, size().y));
	Rectf inter = view.intersection(*rect);
	if(uv){
		Rectf r = *rect;
		Rectf scale = (inter  - r.left_top()) / (r - r.left_top());
		*uv = *uv * scale;
	}
	*rect = inter;
}


void UISpriteEditorViewport::onRedraw()
{
	float zoom = ZoomTable[zoom_];

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
}

void UISpriteEditorViewport::onFinalize()
{
    RELEASE(scene_);
	RELEASE(camera_);
}

void UISpriteEditorViewport::onInitialize()
{
	scene_ = gb_VisGeneric->CreateScene();
	camera_ = scene_->CreateCamera();

	onResize(size().x, size().y);
}

void UISpriteEditorViewport::onResize(int width, int height)
{
	__super::onResize(width, height);
	updateCameraFrustum(size().x, size().y);
}

void UISpriteEditorViewport::updateCameraFrustum(int _width, int _height)
{
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
}

void UISpriteEditorViewport::scroll (const Vect2f& delta)
{
    view_offset_ += delta;
}

void UISpriteEditorViewport::move (const Vect2f& pos, const Vect2i& delta)
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
	signalChanged_.emit();
	//static_cast<UISpriteEditorViewportDlg*>(GetParent ())->UpdateControls ();
}

void UISpriteEditorViewport::onMouseMove()
{
	Vect2i point = mousePosition();
	static Vect2f deltaDiff (Vect2f::ZERO);
	static Vect2i lastPoint = point;
	Vect2i deltaPoint = point - lastPoint;

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
}

Rectf UISpriteEditorViewport::coordsToWindow (const Rectf& rect)
{
    return Rectf(coordsToWindow (Vect2f (rect.left(), rect.top())),
                 rect.size () * ZoomTable[zoom_]);
}

Vect2f UISpriteEditorViewport::coordsFromWindow (const Vect2f& point)
{
    return (point - Vect2f(size()) * 0.5f) / ZoomTable [zoom_] - viewOffset();
}

Rectf UISpriteEditorViewport::coordsFromWindow(const Rectf& rect)
{
	Vect2f left_top(coordsFromWindow(rect.left_top()));
	Vect2f right_bottom(coordsFromWindow(rect.right_bottom()));
    return Rectf(left_top, right_bottom - left_top);
}

Vect2f UISpriteEditorViewport::coordsToWindow (const Vect2f& point)
{
    return (point + viewOffset()) * ZoomTable[zoom_] + Vect2f(size()) * 0.5f;
}

void UISpriteEditorViewport::drawEdgesOverlay(const Rectf& rect)
{
    Color4c guideColor (0, 0, 255, 128);
    Color4c selectedColor (255, 255, 0, 200);
    float zOffset = 12.0f;

	Vect2f pixelSize = Vect2f(3.0f / ZoomTable[zoom_], 3.0f / ZoomTable[zoom_]) ;

	Rectf view_rect(coordsFromWindow(Rectf(0.0f, 0.0f, size().x, size().y)));



	// mask
	Color4c maskColor(255.0f, 0.0f, 0.0f, 64.0f);
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

void UISpriteEditorViewport::onMouseButtonDown(kdw::MouseButton button)
{
	bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	if(button == kdw::MOUSE_BUTTON_LEFT){
		if (!sprite_.texture ())
			return;
		Vect2i point = mousePosition();
		Rectf rect = coordsToWindow (sprite_.textureCoords () * Vect2f (sprite_.texture ()->GetWidth (),
																		sprite_.texture ()->GetHeight ()));
		if(control) {
			Vect2f mousePoint = coordsFromWindow (Vect2f(point.x, point.y));
			Rectf rect = getCoords();
			rect.left(mousePoint.x);
			rect.top(mousePoint.y);
			rect.width(0.0f);
			rect.height(0.0f);
			setCoords(rect);
			selected_edges_ = Recti (0, 0, 1, 1);
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
	}
	else if(button == kdw::MOUSE_BUTTON_RIGHT){
		scrolling_ = true;
		captureMouse();
	}
	else if(button == kdw::MOUSE_BUTTON_LEFT_DOUBLE){
		if(!sprite_.isEmpty() && sprite_.texture()){
			setCoords(Rectf(0, 0, sprite_.texture()->GetWidth(), sprite_.texture()->GetHeight()));
			signalChanged_.emit();
		}
	}
	else if(button == kdw::MOUSE_BUTTON_WHEEL_DOWN){
		zoom_ = clamp(zoom_ + 1, 0, MaxZoom - 1);
		updateCameraFrustum(size().x, size().y);
		redraw();
	}
	else if(button == kdw::MOUSE_BUTTON_WHEEL_UP){
		zoom_ = clamp(zoom_ - 1, 0, MaxZoom - 1);
		updateCameraFrustum(size().x, size().y);
		redraw();
	}
}

void UISpriteEditorViewport::onMouseButtonUp(kdw::MouseButton button)
{
	if(button == kdw::MOUSE_BUTTON_LEFT){
		moving_ = false;
		selected_edges_ = Recti (0, 0, 0, 0);
		redraw();
	}
	else if(button == kdw::MOUSE_BUTTON_RIGHT){
		scrolling_ = false;
		releaseMouse();
	}
}


/*
void UISpriteEditorViewport::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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
*/

void UISpriteEditorViewport::setSprite( const UI_Sprite& sprite )
{
	sprite_ = sprite;
}

UI_Sprite UISpriteEditorViewport::getSprite() const
{
	return sprite_;
}

void UISpriteEditorViewport::setTexture(const char* name)
{
	if(name){
		UI_TextureReference texture (name);
		sprite_.setTextureReference (texture);
	}else{
		UI_TextureReference texture;
		sprite_.setTextureReference (texture);
	}
	redraw ();
}

void UISpriteEditorViewport::nextFrame()
{
	if(sprite_.isAnimated()){
		cTexture* texture = sprite_.texture();
		animationTime_ += 0.001f * float(texture->GetTotalTime()) / float(texture->frameNumber());
		animationPhase_ = sprite_.phase(animationTime_, true);
		redraw();
	}
}

void UISpriteEditorViewport::prevFrame()
{
	if(sprite_.isAnimated()){
		cTexture* texture = sprite_.texture();
		animationTime_ -= 0.001f * float(texture->GetTotalTime()) / float(texture->frameNumber());
		animationPhase_ = sprite_.phase(animationTime_, true);
		redraw();
	}
}

void UISpriteEditorViewport::enableAnimation(bool enable)
{
	animationEnabled_ = enable;
}

void UISpriteEditorViewport::onTimer()
{
	static double lastTime = xclock() * 0.001;
	double currentTime = xclock() * 0.001;
	float delta = min(0.1f, float(currentTime - lastTime));
	
	if(animationEnabled_)
		animationTime_ += delta;

	animationPhase_ = sprite_.phase(animationTime_, true);
	redraw();
	lastTime = currentTime;
}