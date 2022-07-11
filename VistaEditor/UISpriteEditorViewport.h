#ifndef __UI_SPRITE_EDITOR_VIEWPORT__
#define __UI_SPRITE_EDITOR_VIEWPORT__

#include "kdw/Viewport.h"
#include "kdw/Timer.h"
#include "XTL\Rect.h"
#include "UserInterface\UI_Types.h"
#include "Render\inc\IRenderDevice.h"

class Archive;
class UISpriteEditorViewport : public kdw::Viewport{
public:
	UISpriteEditorViewport();
	~UISpriteEditorViewport();

	void serialize(Archive& ar);

    Rectf getCoords() const;
	void setCoords(const Rectf& coords);
	void setTexture(const char* name);

	void nextFrame();
	void prevFrame();
	void enableAnimation(bool enable);

	void setSprite (const UI_Sprite& sprite);
	UI_Sprite getSprite () const;

	sigslot::signal0& signalChanged(){ return signalChanged_; }
	sigslot::signal0& signalTextureChanged(){ return signalTextureChanged_; }
protected:
	void onRedraw();
	void onResize(int width, int height);
	void onTimer();
	void updateCameraFrustum(int width, int height);

    void onInitialize();
    void onFinalize();

	void onMouseMove();
	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);

    void drawRect(const Rectf& _rect, float _depth, const Color4c& _color, eBlendMode _blend_mode = ALPHA_BLEND);
	void drawRectTextured(const Rectf& _rect, float _depth, const Color4c& _color, cTexture* _texture, const Rectf& _uv_rect,  eBlendMode _blend_mode = ALPHA_BLEND, float phase = 0.0f, float saturation = 1.0f);
	
	void drawEdgesOverlay(const Rectf& rect);

	void clipRectByView(Rectf* rect, Rectf* uv);

	Rectf coordsToWindow (const Rectf& rect);
	Rectf coordsFromWindow (const Rectf& rect);
	
	Vect2f coordsToWindow (const Vect2f& point);
	Vect2f coordsFromWindow (const Vect2f& point);

	void scroll(const Vect2f& delta);
    void move(const Vect2f& pos, const Vect2i& delta);

    const Vect2f& viewOffset() const { return view_offset_; }

	UI_Sprite sprite_;

    Camera* camera_;

	int zoom_;
    bool scrolling_;
    bool moving_;
    Recti selected_edges_;
    
    Vect2f view_offset_;

	bool animationEnabled_;
	float animationPhase_;
	float animationTime_;
	ShareHandle<kdw::Timer> timer_;
	sigslot::signal0 signalChanged_;
	sigslot::signal0 signalTextureChanged_;
};

#endif
