#ifndef __CURVE_EDITOR_SPACE_H_INCLUDED__
#define __CURVE_EDITOR_SPACE_H_INCLUDED__

#include "kdw/Space.h"
#include "kdw/Document.h"

namespace kdw{
	class CurveEditor;
	class CheckBox;
	class GradientBar;
	class ModelNode;
}

class CurveEditorSpace : public kdw::Space, kdw::ModelObserver{
public:
	CurveEditorSpace();
	
	void update();
	void onModelChanged(kdw::ModelObserver* changer);
	void onSelectedChanged(kdw::ModelObserver* changer);
	void onCurveToggled();
	void onTimeChanged(kdw::ModelObserver* changer);
	void onCurveEditorTimeChanged();
	void onGradientChanged();

	void onMenuPlaybackRewind();
	void onMenuPlaybackPause();

	void updateMenus();
protected:
	kdw::CurveEditor* curveEditor_;
	kdw::GradientBar* gradientColorBar_;
	kdw::GradientBar* gradientAlphaBar_;
	kdw::CheckBox* pauseCheck_;
	kdw::ModelNode* selectedNode_;
};

#endif
