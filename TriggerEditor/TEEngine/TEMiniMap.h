#ifndef __TE_MINI_MAP_H_INCLUDED__
#define __TE_MINI_MAP_H_INCLUDED__

class TriggerEditor;
class TriggerEditorView;

class TEMiniMap : public CWnd
{
	DECLARE_DYNAMIC(TEMiniMap)
public:
	static const char* className() {
		return "TriggerEditorMiniMapWindow";
	}
	TEMiniMap();           
	TEMiniMap(TriggerEditor* triggerEditor); 

	void rectToMinimap(CRect& rect);
	void pointFromMinimap(CPoint& point);


	void registerWindowClass();  
	void updateView();

	void setView(TriggerEditorView* view);
	virtual ~TEMiniMap();
public:
	BOOL Create(CWnd* parent);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
private:
	CSize windowSize_;


	float scaleX_;
	float scaleY_;

	CPoint viewOrigin_;
	TriggerEditor* triggerEditor_;
	TriggerEditorView* triggerEditorView_;
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

#endif
