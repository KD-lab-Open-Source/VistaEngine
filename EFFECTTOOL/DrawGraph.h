#if !defined(AFX_DRAWGRAPH_H__0A1ED122_17BF_4A09_9D2B_6ED6F99284FC__INCLUDED_)
#define AFX_DRAWGRAPH_H__0A1ED122_17BF_4A09_9D2B_6ED6F99284FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DrawGraph.h : header file
//

#include "EffectToolDoc.h"

struct GRAPH_BOUND
{
	float min;
	float max;
	GRAPH_BOUND(){};
	GRAPH_BOUND(float mn, float mx): min(mn),max(mx){}
};

//базовый класс дл€ обьектов на графике
class CGraphObject
{
protected:
	CGraphObject*  m_pParent; //владелец (дл€ точек - лини€, дл€ линий - 0)

public:
	enum //допустимые операции с обьектом
	{
		TRACK_HORZ = 0x01,
		TRACK_VERT = 0x02,
		TRACK_ALL  = 0x03,
		REMOVEABLE = 0x04
	};

	bool       m_bActive;

	enum eKindObj
	{
		KIND_UNKNOWN,
		KIND_EMITTER_TIME_LINE
	};

	eKindObj kindObject;

	CGraphObject(CGraphObject* p);
	virtual ~CGraphObject(){}

	virtual void Draw(CDC* pDC) = 0;
	virtual CGraphObject* HitTest(const CPoint& pt) = 0;
	virtual void Update(bool bFullUpdate){}
	virtual void OffsetPos(int dx, int dy){}
	virtual void OnChange(){}
	virtual int  GetState(){ return TRACK_HORZ|TRACK_VERT;}
	virtual void OnRemoveObject(CGraphObject* pObject){}
	virtual CGraphObject* GetParent(){return m_pParent;}
	virtual CGraphObject* GetObj(int n){return NULL;}
	virtual int GetNumObj(CGraphObject* Obj){return -1;}
	virtual CPoint& GetPos(){static CPoint t(0,0);return t;}
	virtual void DoubleClick(){}
};

/////////////////////////////////////////////////////////////////////////////
//точка - базовый класс
class CGraphPoint : public CGraphObject
{
protected:
	CPoint     m_pt;
	CBrush*    m_pBrush;
	CBrush*    m_pBrushSelected;

	KeyWrapBase& m_key;
public:

	int        m_nNumber;

	CGraphPoint(CGraphObject* p, int n, KeyWrapBase& key, int x, int y, CBrush* pBr, CBrush* pBrSel);

	virtual void DoubleClick(CPoint& point);
	virtual void Draw(CDC* pDC);
	virtual CGraphObject* HitTest(const CPoint& pt);
	virtual void OffsetPos(int x, int y);

	virtual CPoint& GetPos(){
		return m_pt;
	}
};

/////////////////////////////////////////////////////////////////////////////
//лини€ - базовый класс
class CGraphLine : public CGraphObject
{
	friend class CGraphPoint;
	friend class CBlueLinePoint;
public://protected:
	typedef  TAutoReleaseContainer< vector<CGraphPoint*> >  PointVectorType;
	float koef;
	bool editable;
	PointVectorType m_points;
	bool            m_bVisible;
	bool            m_bCanCreatePoints;
	CPen*           m_pPen;

	KeyWrapBase& m_key;

public:
	virtual CGraphPoint* CreateNewPoint(const CPoint& pt) = 0;
	CGraphLine(KeyWrapBase& key, CPen* pPen);
	virtual CGraphObject* HitTest(const CPoint& pt);
	virtual void Draw(CDC* pDC);
	virtual float GetBoundValue(GRAPH_BOUND& f)=0;
	void CheckHorzPos(CGraphPoint* pPoint);
	virtual int GetNumObj(CGraphObject* Obj);
	virtual CGraphObject* GetObj(int n);
};


/////////////////////////////////////////////////////////////////////////////
//график параметра эмитера
//син€€ лини€ внизу
enum DataLineType {BlueLine, KeyLine, BSLine, HermitLine};
struct DataLine
{
	CKey *pKey;
	BSKey* pKeyBS;
	CKeyPosHermit* splkey;
	DataLineType type;
	float dat;
	bool editable;
	bool red;
	void clear(DataLineType tp, bool ed)
	{
		pKey=NULL;  pKeyBS=NULL; splkey=NULL;
		type = tp;
		editable = ed;
		red = false;
		dat = 1;
	}
	DataLine(DataLineType tp, CKey *pK, bool edt, float dt=1){clear(tp, edt); pKey = pK; dat = dt;}
	DataLine(DataLineType tp, BSKey* BspK, bool edt){clear(tp, edt); pKeyBS = BspK; }
	DataLine(DataLineType tp, CKeyPosHermit* pH, bool edt){clear(tp, edt); splkey = pH;}
};

class CBlueLinePoint : public CGraphPoint
{
public:
	CBlueLinePoint(CGraphObject* p, int n, KeyWrapBase& key, int x, int y);

	virtual void OffsetPos(int x, int y);
	virtual void Draw(CDC* pDC);
	virtual void OnChange();
	virtual int  GetState();
	virtual void DoubleClick();
};
class CGraphBlueLine : public CGraphLine
{
public:
	virtual CGraphPoint* CreateNewPoint(const CPoint& pt);
	int type;
//	float koef;
	CGraphBlueLine(int tp = 0,KeyWrapBase& key  = *(new KeyDummyWrap()),float koef = 1.0f);
	virtual void Update(bool bFullUpdate);
	virtual void OnRemoveObject(CGraphObject* pObject);
	virtual float GetBoundValue(GRAPH_BOUND& f);
};
/////////////////////////////////////////////////////////////////////////////
//график параметра частицы
class CKeyLinePoint : public CGraphPoint
{
	bool    m_bPrimaryKey;
	bool    m_bNoVertTrack;
public:
	CKeyLinePoint(CGraphObject* p, int n, KeyWrapBase& key, int x, int y, bool b1, bool b2);

	virtual void Draw(CDC* pDC);
	virtual void OnChange();
	virtual int  GetState();
	virtual void DoubleClick();
};
class CGraphKeyLine : public CGraphLine
{
	bool    m_bPrimaryKey;
	bool    m_bNoVertTrack;
//	float	koef;
public:
	virtual CGraphPoint* CreateNewPoint(const CPoint& pt);
	CGraphKeyLine(KeyWrapBase& key, bool , bool bNoVertTrack, float k =1);
	~CGraphKeyLine();

	virtual void Update(bool bFullUpdate);
	virtual void OnRemoveObject(CGraphObject* pObject);
	virtual float GetBoundValue(GRAPH_BOUND& f);
};
/////////////////////////////////////////////////////////////////////////////
//график всех эмиттеров
class CEmitterTimeline;
class CTimelineContainer : public CGraphObject
{
	typedef  TAutoReleaseContainer< vector<CEmitterTimeline*> >  ItemVectorType;

	ItemVectorType  m_items;
public:
	CTimelineContainer();

	virtual void Draw(CDC* pDC);
	virtual CGraphObject* HitTest(const CPoint& pt);
	virtual void Update(bool bFullUpdate);
	virtual int GetNumObj(CGraphObject* obj);
	virtual CGraphObject* GetObj(int ix);
};

class CEmitterTimeline : public CGraphObject
{
	typedef vector<CPoint>  PointList;

	int			m_nEmitter;
	PointList	m_points;
	int			m_CurPoint;
	CString		m_name;
	CPoint		m_endPoint;
	bool		m_bEndPoint;
public:
	CEmitterTimeline(CGraphObject* p, int n, LPCTSTR name);

	virtual void Draw(CDC* pDC);
	virtual CGraphObject* HitTest(const CPoint& pt);
	virtual void OffsetPos(int x, int y);
	virtual void Update(bool bFullUpdate);
	virtual void OnChange();
	virtual CPoint& GetPos();
	int IxEmitter(){return m_nEmitter;}
	virtual void DoubleClick();

};


/////////////////////////////////////////////////////////////////////////////
// CDrawGraph
class CDrawGraph : public CWnd
{
	typedef  TAutoReleaseContainer< list<CGraphObject*> >  ObjectVectorType;

	ObjectVectorType m_objects;
	CPoint           m_ptPrevPoint;
	CGraphObject*    m_pTrack;

	CFont            m_FontTick;

	float            m_fLastKeylinesMax;
	float 			 m_fLastTimeMax;
	float            m_fEffectTimelineMax;
	bool             m_bLockKeylinesMaxUpdate;
	bool             m_bLockTimeMaxUpdate;
	bool			 m_bDragging;
	bool			 m_bVertical;

	void RemoveObject(CGraphObject* pObject);
	void DrawAxes(CDC* pDC, const CRect& rcClient);
	void DrawAxisLabels(CDC* pDC, const CRect& rcClient);
	void DrawTime(CDC* pDC, const CRect& rcClient);
public:
	CDrawGraph();
	virtual ~CDrawGraph();

	void Init();

	void Update(bool bFullUpdate);
	void ShowKeys(list<DataLine>& keys, bool b);
//	void ShowKeys(list<CKey*>& keys, BSKey* pKeyBS, bool b, bool last, list<CKeyWithParam>& emit_param = list<CKeyWithParam>());
	//void ShowSplineKey(CKeyPosHermit& key);
	//void ShowLightKey(CKeyPos& key);

	void CalcTransform();
	float GetMaxValue(GRAPH_BOUND* bn =NULL);
	float GetTimescaleMax(bool all = false);
	float GetEffectTimelineMax(){return m_fEffectTimelineMax;}
	void SetVerticalDrag(bool vertical) {m_bVertical = vertical;}
	bool IsVertiacal() {return m_bVertical;}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDrawGraph)
	//}}AFX_VIRTUAL


	// Generated message map functions
protected:
	//{{AFX_MSG(CDrawGraph)
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRAWGRAPH_H__0A1ED122_17BF_4A09_9D2B_6ED6F99284FC__INCLUDED_)
