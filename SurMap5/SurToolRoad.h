#ifndef __SUR_TOOL_ROAD_H_INCLUDED__
#define __SUR_TOOL_ROAD_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

struct sPolygon;
#include "..\terra\road.h"

//struct Vect3s {
//	short x, y, z;
//	Vect3s() {}
//	Vect3s(short x_, short y_, short z_) {x = x_; y = y_; z = z_;}
//	typedef short short3[3];
//	Vect3s(const short3& v) {x = v[0]; y = v[1]; z = v[2];}
//	Vect3s(const Vect3f& v) {x = round(v.x); y = round(v.y); z = round(v.z);}
//};

//struct sRoad{
//	list<sNode> nodeArr;
//};
// CSurToolRoad dialog

class CSurToolRoad : public CSurToolBase, RoadTool
{
	DECLARE_DYNAMIC(CSurToolRoad)

public:
	CSurToolRoad(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolRoad();

	CEScroll m_roadWidth;
	CEScroll m_edgeAngle;
	CEScroll m_sphericH;
	enum eStateRoadMetod {
		SRM_BegEndLinear=0,
		SRM_BegEndSpheric=1,
		SRM_TrackSurface=2,
		SRM_OnlyTexture=3
	};
	eStateRoadMetod state_rd_roadMetod;

	virtual bool CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	virtual bool CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool CallBack_RMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
    virtual bool CallBack_KeyDown (unsigned int keyCode, bool shift, bool control, bool alt);

	virtual bool CallBack_DrawAuxData(void);

	//sRoad road;
	enum eCurRoadOperation{
		CRO_None,
		CRO_MoveNode,
	};
	eCurRoadOperation curRoadOperation;
	TypeNodeIterator curNode_;
	TypeNodeIterator selectNode(const Vect2i& scrCoord);
	bool selectVerge(const Vect2i& scrCoord);
	void correctWidthAngleRange();


// Dialog Data
	int getIDD() const { return IDD_BARDLG_ROAD; }
protected:
	void checkRadioButtonAndStateControlStatus();
	void recalcNodeHeight();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnPuttrack();
	afx_msg void OnBnClickedBtnClearroad();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedBtnBrowseFile();
	afx_msg void OnBnClickedBtnBrowseFilev();
	afx_msg void OnRoadMetodButtonClicked(UINT nID);
	afx_msg void OnBnClickedBtnBrowseFileEdgebitmapTexture();
	afx_msg void OnBnClickedBtnBrowseFileEdgebitmapVtexture();
	afx_msg void OnCbnSelchangeCmbTexturingMethod();
	afx_msg void OnCbnSelchangeCmbPutMethod();
};

#endif
