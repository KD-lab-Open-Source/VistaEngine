#ifndef __SUR_TOOL_UNIT_H_INCLUDED__
#define __SUR_TOOL_UNIT_H_INCLUDED__

#include "..\Units\AttributeReference.h"
#include "..\Util\mfc\SizeLayoutManager.h"
#include "SurToolAux.h"
#include "EScroll.h"
#include "xmath.h"

class Player;
class UnitBase;

class CSurToolUnit : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolUnit)

public:
	CSurToolUnit(CWnd* pParent = NULL);
	virtual ~CSurToolUnit();

	void setUnitID(const AttributeBase* unitAttribute);
	
	UnitBase* unitOnMouse(){ return unitOnMouse_; }

    void setPlayer(Player* _player){
        player_ = _player;
    }

	void quant();

	bool CallBack_OperationOnMap(int x, int y);
    bool CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	void CallBack_ReleaseScene();


	int getIDD() const { return IDD_BARDLG_UNIT; }

	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	void updateUnitOnMouse();
	void updateSeed();
	void calculateUnitPose(Se3f& result);

	unsigned long seed_;
    const AttributeBase* unitAttribute_;
    Player* player_;
    UnitBase* unitOnMouse_;
	RandomGenerator random_;
	CSizeLayoutManager layout_;

	// Controls
	CEScroll2 angleSlider_;
	CEScroll2 angleDeltaSlider_;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

#endif
