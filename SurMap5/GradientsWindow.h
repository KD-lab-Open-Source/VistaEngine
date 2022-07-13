#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
#ifndef __GRADIENTS_WINDOW_H_INCLUDED__
#define __GRADIENTS_WINDOW_H_INCLUDED__

#include "Functor.h"
#include "Handle.h"

enum GradientPositionMode;

class LayoutWindow;
class LayoutVBox;
class CColorSelector;

class CKeyColor;
class CGradientEditorView;
class CGradientPositionCtrl;
class CGradientRulerCtrl;

struct GradientListItem{
	const char* name;
	CKeyColor* gradient;
};
typedef std::vector<GradientListItem> GradientList;

class CGradientsWindow : public CWnd{
	DECLARE_DYNAMIC(CGradientsWindow)
public:
	enum{
		STYLE_NO_COLOR           = 1 << 0,
		STYLE_NO_ALPHA           = 1 << 1,
		STYLE_FIXED_POINTS_COUNT = 1 << 2,
		STYLE_CYCLED             = 1 << 3,
	};

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);

    CGradientsWindow(CWnd* parent = 0);
    virtual ~CGradientsWindow();

	void setGradientList(const GradientList& gradients);

	Functor1<void, int>& signalGradientChanged() { return signalGradientChanged_; }
    
	static const char* className(){ return "VistaEngineGradientsWindow"; }
protected:
	void PreSubclassWindow();

	bool modalResult_;
	int style_;
	int currentGradient_;

	PtrHandle<LayoutWindow> layout_;
	LayoutVBox* gradientsBox_;
	PtrHandle<CColorSelector> colorSelector_;
	PtrHandle<CGradientPositionCtrl> positionControl_;
	PtrHandle<CGradientRulerCtrl> rulerControl_;

    typedef std::list< PtrHandle<CGradientEditorView> > GradientViews;
    typedef std::list< CStatic > Labels;

    GradientList gradients_;
	GradientViews gradientViews_;
	void createGradientViews();
    
	Functor1<void, int> signalGradientChanged_;

	CWnd* parent_;
    DECLARE_MESSAGE_MAP()
private:
	CGradientEditorView* getViewByIndex(int index);
	void onColorChanged();
	void onGradientViewChanged(int index);
	void onPositionChanged(int index);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#endif // __GRADIENT_EDITOR_H_INCLUDED__
