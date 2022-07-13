#ifndef __SUR_TOOL3_D_M_H_INCLUDED__
#define __SUR_TOOL3_D_M_H_INCLUDED__

#include <string>
#include "EScroll.h"
#include "SurToolAux.h"

#include "ObjectSpreader.h"
#include "MFC\SizeLayoutManager.h"
#include "XTL/sigslot.h"
#include "EventListeners.h"

class UnitEnvironment;

class CSurTool3DM : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurTool3DM)
public:
	CSurTool3DM(CWnd* pParent = 0);
	virtual ~CSurTool3DM();

	int m_idxCurAttribute;
	CEScroll2 angleSlider_;
	CEScroll2 angleDeltaSlider_;

	CEScroll2 scaleSlider_;
	CEScroll2 scaleDeltaSlider_;

	CEScroll2 spreadRadiusSlider_;
	CEScroll2 spreadRadiusDeltaSlider_;

	static const int PLACEMETOD_MAX_METODS=4;
	int m_cBoxPlaceMetod;
	int cBoxPlaceMetodArr[PLACEMETOD_MAX_METODS];

	int rbUpDown;
	int rbRelAbs;
	int rbPutDig;

	RandomGenerator random_;
	int seed_;
	Se3f pose;

	typedef std::vector<class cObject3dx*> VisualObjectsList;
	VisualObjectsList visualObjects;
	VisualObjectsList logicObjects;
	class cObject3dx* visualPreviewObject3DM;
	ObjectSpreader objectSpreader;
	bool ReloadM3D();


    void hide2WControls();
    void initControls();
	void updateRBControl(bool flag_init);
    void createModels(const char* fileName, int count);
    void releaseModels();

    Se3f calculateObjectPose(Vect2f position, float radius);

    float getScale();
	float getModelRadius (cObject3dx* model);

	void UpdateShapeModel(void);

	// virtuals:
	void onCreateScene(void);
	void onReleaseScene(void);
	bool onOperationOnMap(int x, int y);
	bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onDrawPreview (int cx, int cy);

	void serialize(Archive& ar);
	bool isLabelEditable() const { return true; }
	bool isLabelTranslatable() const { return false; }

// Dialog Data
	int getIDD() const { return IDD_BARDLG_3DM; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CSizeLayoutManager m_layout;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual afx_msg void OnBnClickedBtnBrowseModel();
	afx_msg void OnDestroy();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnCbnSelchangeCboxAttributes();
	afx_msg void OnCbnSelchangeComboModelshape3d();
	BOOL m_bSpread;
	BOOL m_bVertical;
	afx_msg void OnSpreadCheckClicked();
	afx_msg void OnVerticalCheckClicked();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCbnSelchangeCboxPlacemetod();
	afx_msg void OnRBClicked(UINT nID);
};

class CSurToolEnvironment : public CSurTool3DM, public sigslot::has_slots, public ObjectObserver
{
public:
	bool isLabelEditable() const{ return true; }
	bool isLabelTranslatable() const{ return false; }

	void OnBnClickedBtnBrowseModel();
    bool ReloadM3D ();

	static void savePreset(UnitEnvironment* unit);
	static void loadPreset(UnitEnvironment* unit);

	// virtuals:
	BOOL OnInitDialog();

	void onBrushRadiusChanged();
	bool onOperationOnMap(int x, int y);
	bool onDrawAuxData ();
	bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord);

private:
};

#endif
