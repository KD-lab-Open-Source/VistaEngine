#ifndef __SUR_TOOL_TRANSFORM_H_INCLUDED__
#define __SUR_TOOL_TRANSFORM_H_INCLUDED__

#include "..\Util\MFC\SizeLayoutManager.h"
#include "SurToolAux.h"

class CSurToolTransform : public CSurToolBase{
	DECLARE_DYNAMIC(CSurToolTransform)
public:
	CSurToolTransform(CWnd* pParent = 0);
	virtual ~CSurToolTransform();

	virtual bool CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	virtual bool CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool CallBack_RMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual void CallBack_SelectionChanged ();

	virtual bool CallBack_Delete ();
    virtual bool CallBack_KeyDown (unsigned int keyCode, bool shift, bool control, bool alt);

	virtual bool CallBack_DrawAuxData(void);
	void drawAxis(const Vect3f& point, float radius, bool axis[3]);

	Vect3f transformAxis() const {
		if (transformAxis_[0])
			return Vect3f::I;
		else if (transformAxis_[1])
			return Vect3f::J;
		else
			return Vect3f::K;
	}
	int transformAxisIndex() const {
		if (transformAxis_[0])
			return 0;
		else if (transformAxis_[1])
			return 1;
		else
			return 2;
	}
	void cancelTransformation();
	void finishTransformation();

	virtual void beginTransformation() = 0;

	int getIDD() const { return IDD_BARDLG_TRANSFORM; }
protected:
	virtual void onTransformAxisChanged(int index) {};
	
    // utils
    void drawCircle (const Se3f& position, float radius, const sColor4c& color);

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
protected:
	bool localTransform_;
	Vect2i cursorCoord_;
	Vect3f startPoint_;
	float startRadius_;
	Vect3f endPoint_;

	Vect3f selectionCenter_;
	float selectionRadius_;

	bool transformAxis_[3];
    std::vector<PoseRadius> poses_;
    
	bool buttonPressed_;
	/*
	Vect2i begWorldCoord;
	Vect2i begScrCoord;
	Vect2i endScrCoord;
	*/
    CSizeLayoutManager layout_;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	
	afx_msg void OnToSurfaceLevel();
	afx_msg void OnToSurfaceNormal();

	afx_msg void OnXAxisCheck();
	afx_msg void OnYAxisCheck();
	afx_msg void OnZAxisCheck();
};


#endif
