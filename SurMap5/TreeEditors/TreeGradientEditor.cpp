#include "StdAfx.h"
#include "umath.h"
#include "EditArchive.h"

#include "..\..\Render\src\Gradients.h"
#include "GradientEditorWindow.h"
#include "GradientEditor.h"
#include "GradientPositionCtrl.h"

template<GradientPositionMode positionMode, int colorMode, class GradientType>
class TreeGradientEditorTemplate : public TreeEditorImpl<TreeGradientEditorTemplate, GradientType> {
public:
	bool invokeEditor(CKeyColor& keyColor, HWND wnd)
	{
		CWnd* parent = CWnd::FromHandle(wnd);
		CGradientEditorWindow window(positionMode, colorMode, keyColor, parent ? parent->GetParent() : 0);
		
		if(window.doModal())
			keyColor = window.getGradient();
		return true;
	}
	bool postPaint(HDC hdc, RECT rt) {
		CDC dc;
		dc.Attach(hdc);
		InflateRect(&rt, -1, -1);
		rt.bottom -= 1;
		CGradientEditorView::DrawGradient(&dc, rt, getData ());
		dc.Detach();
		return false;
	}
	bool hideContent () const { return true; }
	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }
};


#define REGISTER_GRADIENT_EDITOR(type, position_mode, style) typedef TreeGradientEditorTemplate<position_mode, style, type> TreeGradientEditor##type; \
															             REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(type).name(), TreeGradientEditor##type);


REGISTER_GRADIENT_EDITOR(CKeyColor,                   GRADIENT_POSITION_FLOAT, 0)
REGISTER_GRADIENT_EDITOR(TimeNoAlphaGradient,         GRADIENT_POSITION_TIME,  CGradientEditorWindow::STYLE_NO_ALPHA)
REGISTER_GRADIENT_EDITOR(BytePositionNoColorGradient, GRADIENT_POSITION_BYTE,  CGradientEditorWindow::STYLE_NO_COLOR)
REGISTER_GRADIENT_EDITOR(WaterGradient,               GRADIENT_POSITION_BYTE,  CGradientEditorWindow::STYLE_NO_COLOR | CGradientEditorWindow::STYLE_FIXED_POINTS_COUNT)
REGISTER_GRADIENT_EDITOR(SkyGradient,                 GRADIENT_POSITION_TIME,  CGradientEditorWindow::STYLE_NO_ALPHA | CGradientEditorWindow::STYLE_CYCLED)
REGISTER_GRADIENT_EDITOR(SkyAlphaGradient,                 GRADIENT_POSITION_TIME, CGradientEditorWindow::STYLE_CYCLED)

#undef REGISTER_GRADIENT_EDITOR
