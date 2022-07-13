#ifndef __CONDITION_EDITOR_H_INCLUDED__
#define __CONDITION_EDITOR_H_INCLUDED__

#include "Rect.h"
#include "xmath.h"
#include "IVisGeneric.h"

#include "ViewManager.h"

#include "EditableCondition.h"
#include "IConditionDroppable.h"

struct ConditionSlot {
	static float SLOT_HEIGHT;
	static float TEXT_HEIGHT;

	ConditionSlot();

	void create (Condition* condition, int offset = 0, ConditionSlot* parent = 0);

    int height() const;
	int depth() const;
    void calculateRect(const Vect2f& point, int parent_depth = 0);

	void invert();
	bool inverted() const { return condition_ ? condition_->inverted() : false; }

	typedef std::list<ConditionSlot> Slots;

	ConditionSlot* parent() { return parent_; }
	Condition* condition() { return condition_; }
	const Condition* condition() const{ return condition_; }
	const Rectf& rect() const { return rect_; }
	const Rectf& text_rect() const { return text_rect_; }
	const Rectf& not_rect() const { return not_rect_; }
	const char* name() const { return name_.c_str(); }
	void setName(const char* name) { name_ = name; }
	Slots& children() { return children_; }
	const Slots& children() const{ return children_; }
protected:

	std::string name_;
	Slots children_;
    sColor4f color_;
    int offset_;

	Rectf rect_;
	Rectf text_rect_;
	Rectf not_rect_;
	struct Condition* condition_;
	ConditionSlot* parent_;
private:
	void operator=(const ConditionSlot& rhs) {
	}
};



struct ConditionSlot;

class CConditionEditor : public CWnd, public IConditionDroppable
{
	DECLARE_DYNAMIC(CConditionEditor)
	friend class CConditionEditorDlg;
public:

	enum TextAlign {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};


	CConditionEditor();
	virtual ~CConditionEditor();

	static const char* className() { return "VistaEngineConditionEditor"; }

	void redraw (CDC& dc);

	virtual void beginExternalDrag (const CPoint& point, int typeIndex);

	void onDrop (const CPoint& point, int typeIndex);
	void onDrop (const CPoint& point, Condition* condition);
	bool canBeDropped(const CPoint& point);
	void trackMouse (const Vect2f& point, bool dragOver = false);

	void setCondition (const EditableCondition& condition);
	const EditableCondition& condition () const { return condition_; }
protected:
	ShareHandle<Condition> pasteCondition();
	void copyCondition(Condition* condition);

	Condition* conditionUnderMouse();

	void drawCircle (CDC& dc, const Vect2f& pos, float radius, const sColor4c& color, int outline_width);
	void drawLine (CDC& dc, const Vect2f& _start, const Vect2f& _end, const sColor4c& _color);
	void drawRectangle (CDC& dc, const Rectf& rect, const sColor4c& color);
	void fillRectangle (CDC& dc, const Rectf& rect, const sColor4c& color);
	void drawText (CDC& dc, const Rectf& rect, const char* text, TextAlign align = ALIGN_CENTER);

	bool canBeDropped (Condition *dest, Condition* source);

	float pixelWidth () const;
	float pixelHeight () const;

	void drawSlot (CDC& dc, const ConditionSlot& slot);
	void createFont ();
    
	DECLARE_MESSAGE_MAP()
private:
	CFont m_fntPosition;

	Vect2f click_point_;
	Vect2f cursorPosition_;

	bool scrolling_;
	bool moving_;
	bool draging_;

    ConditionSlot currentSlots_;
	Condition* selectedCondition_;
	Condition* dragedCondition_;
	int newConditionIndex_;
	EditableCondition condition_;
    ViewManager view_;
private:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();

	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	virtual void PreSubclassWindow();
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#endif
