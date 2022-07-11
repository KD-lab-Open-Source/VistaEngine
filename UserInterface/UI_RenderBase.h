#ifndef __UI_RENDER_BASE_
#define __UI_RENDER_BASE_

#include "XTL\Rect.h"
#include "XMath\Colors.h"
#include "Render\inc\IRenderDevice.h"

class UI_Sprite;
class cTexture;

class UI_RenderBase
{
public:
	static void create();

	cTexture* createTexture(const char* file_name) const;
	void releaseTexture(cTexture* &texture) const;

	void setWindowPosition(const Recti& pos) { windowPosition_ = pos; }
	const Recti& windowPosition() const { return windowPosition_; }

	void setRenderSize(const Vect2i& size) { renderSize_ = size; }
	const Vect2i& renderSize() const{ return renderSize_; }

	Vect2i defaultResolution() const { return defaultResolution_; }
	float resolutionAspect() const { return float(defaultResolution_.x) / float(defaultResolution_.y); }

	void drawSprite(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, float saturation, eBlendMode blendMode, float phase = 0.0f) const;
	void drawSpriteTiled(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, float saturation, eBlendMode blendMode, float phase = 0.0f) const;
	void drawSpriteSolid(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, eBlendMode blendMode, float phase = 0.0f) const;

	void drawLine(const Vect2f& p0, const Vect2f& p1, const Color4f& color) const;
	
	void drawRectangle(const Rectf& rect, const Color4f& color = Color4f::WHITE, bool outlined = false/*, UI_BlendMode blend_mode = UI_BLEND_NORMAL*/) const;


	/// пересчёт из экранных в координаты относительно окна рендера 
	Vect2f deviceCoords(const Vect2i& screen_coords) const;
	/// пересчёт из экранных в координаты относительно окна рендера 
	Rectf deviceCoords(const Recti& screen_coords) const;

	/// пересчёт из относительных координат в экранные
	Vect2i screenCoords(const Vect2f& rel_coords) const{
		Vect2f vec = rel_coords * Vect2f(windowPosition_.size());
		return Vect2i(vec.xi(), vec.yi()) + windowPosition_.left_top();
	}
	/// пересчёт из относительных координат в экранные
	Recti  screenCoords(const Rectf& rel_coords) const{
		Rectf rect = rel_coords * Vect2f(windowPosition_.size());
		return Recti(round(rect.left()) + windowPosition_.left(), 
			round(rect.top()) + windowPosition_.top(),
			max(1, round(rect.width())), max(1, round(rect.height())));
	}
	/// пересчёт из относительного размера в экранный
	Vect2i screenSize(const Vect2f& rel_size) const {
		return Vect2i(
			round(rel_size.x * float(windowPosition_.width())),
			round(rel_size.y * float(windowPosition_.height())));
	}

	/// пересчёт из координат окна рендера в экранные
	Recti device2screenCoords(const Rectf& device_coords) const;

	/// пересчёт из экранных координат в относительные
	Vect2f relativeCoords(const Vect2i& screen_coords) const{
		return Vect2f(screen_coords - windowPosition_.left_top())
			* Vect2f(1.0f / (float)windowPosition_.width(), 1.0f / (float)windowPosition_.height());
	}
	Vect2f relativeCoords(const Vect2f& screen_coords) const{
		return Vect2f(screen_coords - Vect2f(windowPosition_.left_top()))
			* Vect2f(1.0f / (float)windowPosition_.width(), 1.0f / (float)windowPosition_.height());
	}
	/// пересчёт из экранных координат в относительные
	Rectf relativeCoords(const Recti& screen_coords) const {
		Vect2f top_left = relativeCoords(screen_coords.left_top());
		Vect2f right_bottom = relativeCoords(screen_coords.right_bottom());
		return Rectf(top_left, right_bottom - top_left);
	}
	/// пересчёт из экранного размера в относительный
	Vect2f relativeSize(const Vect2i& screen_size) const {
		return Vect2f(screen_size.x / float(windowPosition_.width()), screen_size.y / float(windowPosition_.height()));
	}

	/// пересчёт из координат окна рендера в относительные экранные
	Vect2f device2relativeCoords(const Vect2f& device_coords) const;

	/// пересчёт из относительных в координаты окна рендера
	Vect2f relative2deviceCoords(const Vect2f& relative_coords) const { return deviceCoords(screenCoords(relative_coords));	}
	Rectf relative2deviceCoords(const Rectf& relative_coords) const { return deviceCoords(screenCoords(relative_coords)); }

	static UI_RenderBase& instance()
	{ 
		xassert(self_);
		return *self_;
	}

	virtual ~UI_RenderBase() { self_ = 0; }

protected:
	static UI_RenderBase* self_;

	/// экранные координаты области, доступной для рисования
	Recti windowPosition_;

	Vect2i defaultResolution_;
	Vect2i renderSize_;

	UI_RenderBase();
};

#endif //__UI_RENDER_BASE_