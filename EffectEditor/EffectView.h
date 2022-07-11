#ifndef __VIEW_3DX_MAIN_VIEW_H_INCLUDED__
#define __VIEW_3DX_MAIN_VIEW_H_INCLUDED__

#include "kdw/ControllableViewport.h"
#include "kdw/Space.h"

class Camera;
class cScene;
class cObject3dx;
class EffectDocument;

class EffectView : public kdw::ToolViewport{
public:
	explicit EffectView(bool continuousUpdate = false);
	~EffectView();

	void onIntialize();
	void onRedraw();

	void setShowGrid(bool showGrid);
	void setShowOverdraw(bool overdraw);
	void setShowTerrain(bool showTerrain);
	void setBackgroundColor(const Color4f& color);
	void setEnableLighting(bool enableLighting);

	EffectDocument* document();
protected:

	bool showGrid_;
	bool showOverdraw_;
	bool showTerrain_;
	bool enableLighting_;
};

class EffectViewSpace : public kdw::Space{
public:
	typedef EffectViewSpace Self;
	EffectViewSpace();
	~EffectViewSpace();

	void updateMenus();

	void onMenuViewFocusEffect();
	void onMenuViewFocusAll();
	void onMenuViewFocusSelected();

	void onMenuViewBackgroundColor();
	void onMenuViewShowGrid();
	void onMenuViewShowTerrain();
	void onMenuViewEnableLighting();
	void onMenuViewShowOverdraw();
	void onMenuViewLoadTerrain();

	void serialize(Archive& ar);
protected:
	ShareHandle<EffectView> view_;

	bool showGrid_;
	bool showTerrain_;;
	bool showOverdraw_;
	bool enableLighting_;
	Color4f backgroundColor_;
};

#endif
