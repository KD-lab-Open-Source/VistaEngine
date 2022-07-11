#ifndef __UI_RENDER_H__
#define __UI_RENDER_H__

#include "UI_RenderBase.h"
#include "XTL\Rect.h"
#include "Render\3dx\UMath.h"
#include "XMath\Colors.h"
#include "XTL\SafeCast.h"

#include "UI_Enums.h"

class cVisGeneric;
class cInterfaceRenderDevice;

class cTexture;
class Camera;

class UI_Sprite;
class UI_Font;
class UI_TextFormat;

class UI_TextParser;
struct OutNode;
typedef vector<OutNode> OutNodes;

/// Интерфейс для UI к графическим объектам.
class UI_Render : public UI_RenderBase
{
public:
	static void create();
	void init();
	
	void releaseResources();

	void drawSprite(const Rectf& pos, const UI_Sprite& sprite, const Color4f& color = Color4f::WHITE, UI_BlendMode blend_mode = UI_BLEND_NORMAL, bool tiled = false, float phase = 0.f) const;
	void drawSprite(const Rectf& pos, const UI_Sprite& sprite, float alpha, bool tiled = false, float phase = 0.f) const { drawSprite(pos, sprite, Color4f(1.f, 1.f, 1.f, alpha), UI_BLEND_NORMAL, tiled, phase); }

	void drawSprite(const Rectf& pos, cTexture* texture, const Color4f& color = Color4f::WHITE, UI_BlendMode blend_mode = UI_BLEND_NORMAL, float phase = 0.f) const;
	void drawSprite(const Rectf& pos, cTexture* texture, const Rectf& txt_pos, const Color4f& color, UI_BlendMode blend_mode) const;

	/// dir == true, продублировать wild chars, для корректного отображения
	/// dir == false, убрать продублированные wild chars для возможности редактирования
	Vect2f outText(const Rectf& pos, const wchar_t* text, const UI_TextFormat* = 0, int textAlign = 0, const UI_Font* font = 0, float alpha = 1.f, bool formatText = false) const;
	Vect2f outText(const Rectf& pos, const UI_TextParser& parser, OutNodes::const_iterator begin, OutNodes::const_iterator end, const UI_TextFormat* format, int textAlign, float alpha, bool clampInRange = false) const;
	void outDebugText(const Vect2f& pos, const char* text, const Color4c* color = 0) const;
	
	Vect2f textSize(const wchar_t* text, const UI_Font* font, float fitIn = 0.f) const;
	static void reparseWildChars(bool dir, const wchar_t* in, wstring& out);
	std::string extractFirstLineText(const wchar_t* taggedText) const;

	void setCamera(Camera* camera);
	Camera* camera() { return camera_; }
	void updateRenderSize();
	void setWindowPosition(const Recti& window_pos);
	void setWindowPosition(const Vect2i& window_size){ setWindowPosition(Recti(window_size)); }

	/// возвращает координаты краев экрана, в относительных координах
	Rectf entireScreenCoords() const;
	/// возвращает отступ до края экрана, в относительных координах
	float screenBorderLeft() const;
	float screenBorderTop() const;
	float screenBorderRight() const;
	float screenBorderBottom() const;

	void createFonts(bool reCreateForce = false);
	const UI_Font* defaultFont() const;

	void serialize(Archive& ar);

	static UI_Render& instance()
	{ 
		xassert(self_);
		return *safe_cast<UI_Render*>(self_);
	}

	~UI_Render();
	
private:
	UI_Render();
	
	UI_Font* defaultFont_;
	Camera* camera_;
};

#endif /* __UI_RENDER_H__ */
