#include "XUtil.h"
#include "StdAfx.h"
#include "TreeEditor.h"
#include "Umath.h"
#include "EditArchive.h"

#include "ColorSelectorWindow.h"

static COLORREF toColorRef (const sColor4f& color){
    return RGB(round(color.r * 255.0f), round(color.g * 255.0f), round(color.b * 255.0f));
}

static void fromColorRef(sColor4f& dest, COLORREF src){
    struct COLORREF_STRUCT { unsigned char r, g, b, a; };
    dest.r = static_cast<float>(reinterpret_cast<COLORREF_STRUCT&>(src).r) / 255.0f;
    dest.g = static_cast<float>(reinterpret_cast<COLORREF_STRUCT&>(src).g) / 255.0f;
    dest.b = static_cast<float>(reinterpret_cast<COLORREF_STRUCT&>(src).b) / 255.0f;
}

static COLORREF toColorRef (const sColor4c& color){
    return RGB(color.r, color.g, color.b);
}

static COLORREF toColorRef (const sColor3c& color){
    return RGB(color.r, color.g, color.b);
}

static void fromColorRef(sColor4c& dest, COLORREF src){
    struct COLORREF_STRUCT { unsigned char r, g, b, a; };
    dest.r = reinterpret_cast<COLORREF_STRUCT&>(src).r;
    dest.g = reinterpret_cast<COLORREF_STRUCT&>(src).g;
    dest.b = reinterpret_cast<COLORREF_STRUCT&>(src).b;
}

static void fromColorRef(sColor3c& dest, COLORREF src){
    struct COLORREF_STRUCT { unsigned char r, g, b, a; };
    dest.r = reinterpret_cast<COLORREF_STRUCT&>(src).r;
    dest.g = reinterpret_cast<COLORREF_STRUCT&>(src).g;
    dest.b = reinterpret_cast<COLORREF_STRUCT&>(src).b;
}

static void toColor4f(sColor4f& result, const sColor4f& src){
    result = src;
}

static void toColor4f(sColor4f& result, const sColor4c& src){
    result = sColor4f(src);
}

static void toColor4f(sColor4f& result, const sColor3c& src3c){
    sColor4c src(src3c.r, src3c.g, src3c.b, 255);
    result = sColor4f(src);
}

static bool hasAlpha(const sColor3c& color){ return false; }
static bool hasAlpha(...) { return true; }

//////////////////////////////////////////////////////////////////////////////
template<class ColorType>
class TreeColorSelector : public TreeEditorImpl<TreeColorSelector<ColorType>, ColorType> {
public:
    bool invokeEditor (ColorType& color, HWND parent){
        struct COLORREF_STRUCT { unsigned char r, g, b, a; };
		CWnd* window = CWnd::FromHandle(::GetParent(parent));
		CColorSelectorWindow selector(hasAlpha(color), window);
		sColor4f colorf;
		toColor4f(colorf, color);
		selector.setColor(colorf);
        if(selector.doModal(window))
			color = ColorType(selector.getColor());
        return true;
    }
    bool hideContent () const { return true; }

	Icon buttonIcon()const { return TreeEditor::ICON_DOTS; }

    bool prePaint (HDC dc, RECT rt){
		_CrtCheckMemory();
        InflateRect (&rt, -1, -1);
        rt.bottom -= 2;
		xassert(_CrtIsValidHeapPointer(this));
        HBRUSH hColorBrush = ::CreateSolidBrush(toColorRef(getData()));
        ::FillRect (dc, &rt, hColorBrush);
        ::DeleteObject (hColorBrush);
		_CrtCheckMemory();
        return true;
    }
};

typedef TreeColorSelector<sColor4f> TreeColorSelector4f;
REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(sColor4f).name (), TreeColorSelector4f);

typedef TreeColorSelector<sColor4c> TreeColorSelector4c;
REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(sColor4c).name (), TreeColorSelector4c);

typedef TreeColorSelector<sColor3c> TreeColorSelector3c;
REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(sColor3c).name (), TreeColorSelector3c);
