#ifndef __FORMATION_EDITOR_H_INCLUDED__
#define __FORMATION_EDITOR_H_INCLUDED__

struct sColor4c;

class FormationPattern;

class CFormationEditor : public CWnd
{
	DECLARE_DYNAMIC(CFormationEditor)
	friend class CFormationEditorDlg;
public:
	CFormationEditor();
	virtual ~CFormationEditor();

	void redraw (CDC& dc);

	static const char* className() { return "VistaEngineFormationEditor"; }

	void drawCircle (CDC& dc, const Vect2f& pos, float radius, const sColor4c& color, int outline_width);
	void drawLine (CDC& dc, const Vect2f& _start, const Vect2f& _end, const sColor4c& _color);
	float viewScale () const;
	Vect2f coordsToScreen (const Vect2f& pos) const;
	Vect2f coordsFromScreen (const Vect2f& pos) const;

	void selectCellUnderPoint (const Vect2f& point);

	void setShowTypeNames(bool showTypeNames) { showTypeNames_ = showTypeNames; };
	bool showTypeNames() const{ return showTypeNames_; }
	void setPatternName(const char* name);
	void setPattern(const FormationPattern& pattern);
	const FormationPattern& pattern () const { return pattern_; }
    
	DECLARE_MESSAGE_MAP()
private:
	CFont m_fntPosition;

	Vect2f view_offset_;
	float view_scale_;
	Vect2i view_size_;

	Vect2f click_point_;

	bool scrolling_;
	bool moving_;
	bool showTypeNames_;

	int selected_cell_;

	int zoom_index_;

	FormationPattern& pattern_;
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

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PreSubclassWindow();
public:
	afx_msg void OnFormationCellDelete();
};



#endif
