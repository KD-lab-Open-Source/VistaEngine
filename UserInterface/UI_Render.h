#ifndef __UI_RENDER_H__
#define __UI_RENDER_H__

#include "Handle.h"
#include "Rect.h"
#include "..\Render\inc\UMath.h"

#include "UI_Enums.h"
#include "UI_TextParser.h"

class cVisGeneric;
class cInterfaceRenderDevice;

class cTexture;
class cFont;
class cCamera;
struct sColor4f;

class UI_Sprite;
class UI_Font;
class UI_TextFormat;

/// Интерфейс для UI к графическим объектам.
class UI_Render
{
public:
	cTexture* createTexture(const char* file_name) const;
	bool releaseTexture(cTexture* &texture) const;

	cFont* createFont(const char* font_name, int font_size, const char* path = 0) const;
	bool releaseFont(cFont* font) const;

	void drawSprite(const Rectf& pos, const UI_Sprite& sprite, float alpha, float phase = 0.f) const
		{drawSprite(pos, sprite, sColor4f(1.f, 1.f, 1.f, alpha), UI_BLEND_NORMAL, phase); }
	void drawSprite(const Rectf& pos, const UI_Sprite& sprite, sColor4f color = sColor4f (1.0f, 1.0f, 1.0f, 1.0f), UI_BlendMode blend_mode = UI_BLEND_NORMAL, float phase = 0.f) const;
	void drawSprite(const Rectf& pos, cTexture* texture, sColor4f color = sColor4f (1.0f, 1.0f, 1.0f, 1.0f), UI_BlendMode blend_mode = UI_BLEND_NORMAL, float phase = 0.f) const;
	void drawSprite(const Rectf& pos, cTexture* texture, const Rectf& txt_pos, sColor4f color, UI_BlendMode blend_mode) const;
	void drawLine(const Vect2f& p0, const Vect2f& p1, const sColor4f& color) const;
	void drawRectangle(const Rectf& rect, const sColor4f& color = sColor4f(1,1,1,1), bool outlined = false/*, UI_BlendMode blend_mode = UI_BLEND_NORMAL*/) const;

	void drawMiniMap(const Vect2f& v1,const Vect2f& v2,const Vect2f& v3,const Vect2f& v4, cTexture* baseTexture, bool drawFog, bool drawWater, const sColor4f& borderColor, float Alpha,const sColor4f& terraColor);
	bool setFont(const UI_Font*) const;
	void setFont() const;
	const UI_Font* defaultFont() const;


	void createFonts();

	bool createDefaultFont();
	bool releaseDefaultFont();

	/// dir == true, продублировать wild chars, для корректного отображения
	/// dir == false, убрать продублированные wild chars для возможности редактирования
	static void reparseWildChars(bool dir, const char* in, string& out);
	Vect2f outText(const Rectf& pos, const char* text, const UI_TextFormat* = 0, int textAlign = 0, const UI_Font* font = 0, float alpha = 1.f, bool formatText = false) const;
	Vect2f outText(const Rectf& pos, const UI_TextParser& parser, OutNodes::const_iterator begin, OutNodes::const_iterator end, const UI_TextFormat* format, int textAlign, const UI_Font* font, float alpha, bool clampInRange = false) const;
	void outDebugText(const Vect2f& pos, const char* text, const sColor4c* color = 0) const;
	Vect2f textSize(const char* text, const UI_Font* font, float fitIn = 0.f) const;
	std::string extractFirstLineText(const char* taggedText) const;

	void setInterfaces(cVisGeneric* vis_generic, cInterfaceRenderDevice* render_device);
	cVisGeneric* visGeneric() const { return visGeneric_; }
	cInterfaceRenderDevice* renderDevice() const { return renderDevice_; }

	void setCamera(cCamera* camera);
	cCamera* camera() { return camera_; }
	void updateRenderSize();
	void setWindowPosition(const Recti& window_pos);
	void setWindowPosition(const Vect2i& window_size){ setWindowPosition(Recti(window_size)); }

	const Recti& windowPosition() const { return windowPosition_; }
	const Vect2i& renderSize() const{ return renderSize_; }

	Vect2i defaultResolution() const { return defaultResolution_; }
	float resolutionAspect() const { return float(defaultResolution_.x) / float(defaultResolution_.y); }

	/// возвращает координаты краев экрана, в относительных координах
	Rectf entireScreenCoords() const;

	/// возвращает отступ до края экрана, в относительных координах
	float screenBorderLeft() const;
	float screenBorderTop() const;
	float screenBorderRight() const;
	float screenBorderBottom() const;

	/// возвращает текущие координаты курсора
	Vect2f mousePosition() const;

	/// пересчёт из относительных координат в экранные
	Vect2i screenCoords(const Vect2f& rel_coords) const;
	/// пересчёт из относительных координат в экранные
	Recti  screenCoords(const Rectf& rel_coords) const;

	/// пересчёт из координат окна рендера в экранные
	Recti device2screenCoords(const Rectf& device_coords) const;

	/// пересчёт из экранного размера в относительный
	Vect2f relativeSize(const Vect2i& screen_size) const;
	/// пересчёт из относительного размера в экранный
	Vect2i screenSize(const Vect2f& rel_size) const;
	/// пересчёт из экранных координат в относительные
    Vect2f relativeCoords(const Vect2i& screen_coords) const;
	/// пересчёт из экранных координат в относительные
    Rectf relativeCoords(const Recti& screen_coords) const;

	/// пересчёт из экранных в координаты относительно окна рендера 
    Vect2f deviceCoords(const Vect2i& screen_coords) const;
	/// пересчёт из экранных в координаты относительно окна рендера 
    Rectf deviceCoords(const Recti& screen_coords) const;

	/// пересчёт из координат окна рендера в относительные экранные
    Vect2f device2relativeCoords(const Vect2f& device_coords) const;

	/// пересчёт из относительных в координаты окна рендера
	Vect2f relative2deviceCoords(const Vect2f& relative_coords) const;
	Rectf relative2deviceCoords(const Rectf& relative_coords) const;

	/// возвращает размеры спрайта относительно экрана
	Vect2f relativeSpriteSize(const UI_Sprite& sprite) const;

	void setFontDirectory(const char* path);

	static UI_Render& instance(){ return Singleton<UI_Render>::instance(); }

private:
	UI_Font* defaultFont_;

	/// путь к шрифтам
	std::string fontDirectory_;

	/// экранные координаты области, доступной для рисования
	Recti windowPosition_;

	Vect2i defaultResolution_;
	Vect2i renderSize_;

	cTexture* screenTexture_;
	cCamera* camera_;

	mutable cVisGeneric* visGeneric_;
	mutable cInterfaceRenderDevice* renderDevice_;

	friend Singleton<UI_Render>;
	class PSMiniMap *miniMap;

	UI_Render();
	~UI_Render();
};

#endif /* __UI_RENDER_H__ */
