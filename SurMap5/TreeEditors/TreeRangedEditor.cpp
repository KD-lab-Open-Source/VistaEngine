#include "StdAfx.h"
#include "TreeEditor.h"
#include "EditArchive.h"

//#include "RangedWrapper.h"

namespace{

const int HANDLE_WIDTH = 6;

void makeHandleRect(RECT& rect, float scale)
{
	int width = rect.right - rect.left - HANDLE_WIDTH;
        
	rect.left = rect.left + HANDLE_WIDTH / 2 + round(float(width) * scale);
	rect.right = rect.left + HANDLE_WIDTH;
}

void makeSliderRect(RECT& rect){
	rect.left += round(float(rect.right - rect.left) / 3.0f);
    InflateRect(&rect, -1, -1);
    rect.bottom -= 1;
}

float positionOnSlider(const POINT& point, const RECT& rect){
	float width = rect.right - rect.left - HANDLE_WIDTH;
	return clamp(float(rect.left + HANDLE_WIDTH / 2 - point.x) / width, 0.0f, 1.0f);
}

}
//template<class T>
//class TreeRangedEditor : public TreeEditorImpl<TreeRangedEditor, RangedWrapper<T> > {
//public:
//	bool invokeEditor(RangedWrapper<T>& ranged, HWND wnd) {
//
//		return true;
//	}
//
//	/*
//	bool onMouseDown(RangedWrapper<T>& ranged, POINT point, MouseButton button, const RECT& rect){
//		if(button = BUTTON_LEFT){
//			float scale = (ranged.value() - ranged.range().minimum()) / ranged.range().length();
//			RECT sliderRect = rect;
//			makeSliderRect(sliderRect);
//			RECT handleRect = sliderRect;
//			makeHandleRect(handleRect, scale);
//
//			if(::PtInRect(&handleRect, point)){
//
//			}
//			else if(::PtInRect(&sliderRect, point)){
//				float value = positionOnSlider(point, sliderRect);
//	            
//				return true;
//			}
//		}
//		else
//			return false;
//	}
//	*/
//
//	bool postPaint(HDC hdc, RECT rect) {
//		const RangedWrapper<T>& ranged = getData();
//
//		float scale = (ranged.value() - ranged.range().minimum()) / ranged.range().length();
//		RECT sliderRect = rect;
//		makeSliderRect(sliderRect);
//		RECT handleRect = sliderRect;
//		makeHandleRect(handleRect, scale);
//
//		CDC dc;
//		dc.Attach(hdc);
//		dc.FillSolidRect(&sliderRect, RGB(255, 255, 255));
//		dc.FillSolidRect(&handleRect, RGB(0, 0, 0));
//
//		//CGradientEditor::DrawGradient(&dc, rect, getData());
//
//		dc.Detach();
//		return false;
//	}
//	
//	bool hideContent() const{
//		return true;
//	}
//
//	std::string nodeValue() const{
//		XBuffer buf;
//		buf <= getData().value();
//		return static_cast<const char*>(buf);
//	}
//
//
//	Icon buttonIcon() const{ return TreeEditor::ICON_NONE; }
//};
//
//typedef TreeRangedEditor<float> TreeRangedEditorf;
//typedef TreeRangedEditor<int> TreeRangedEditori;

//REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(RangedWrapperf).name (), TreeRangedEditorf);
//REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(RangedWrapperi).name (), TreeRangedEditori);
