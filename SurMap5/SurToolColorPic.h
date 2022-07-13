#ifndef __SURTOOLCOLORPIC_H__
#define __SURTOOLCOLORPIC_H__

#include "EScroll.h"
#include "SurToolAux.h"
#include "ColorButton.h"
#include "mfc\SizeLayoutManager.h"

// CSurToolColorPic dialog

class CSurToolColorPic : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolColorPic)

public:
	CSurToolColorPic(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolColorPic();

	CEScroll m_CenterAlpha;
	int state_RButton_DrawErase;
	CEScrollVx m_FilterMinH;
	CEScrollVx m_FilterMaxH;
	static int filterMinHValue;
	static int filterMaxHValue;
	static bool flag_EnableFilterH;
	Color4c txColor;
	CColorButton btnSelectColor;
	CEScroll m_KColor;
	CEScroll m_Saturation;
	CEScroll m_Brightness;


	Vect2i previewTextureSize_;
	cTexture* previewTexture_;
// virtuals
	void onCreateScene(void);
	void onReleaseScene(void);
	bool onOperationOnMap(int x, int y);
	bool onDrawAuxData(void);
	bool onDrawPreview(int, int);
	bool isLabelEditable() const { return true; }
	bool isLabelTranslatable() const { return false; }
/////
	bool ReLoadBitmap();
	void UpdateTexture();

	unsigned long* pBitmap;

	void serialize(Archive& ar);
	static void staticSerialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_COLORPIC; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CSizeLayoutManager layout_;
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnBrowseBitmap();
	afx_msg void OnBnClickedBtnSelectColor();
	afx_msg void OnBnClicked_Put2World();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedRbutDrawerase1();
	afx_msg void OnBnClickedRbutDrawerase2();
	afx_msg void OnBnClickedCheckEnableHFilter();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSize(UINT type, int cx, int cy);
};


#endif //__SURTOOLCOLORPIC_H__
