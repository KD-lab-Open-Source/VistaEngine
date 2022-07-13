#ifndef __SURTOOLCOLORPIC_H__
#define __SURTOOLCOLORPIC_H__

#include "EScroll.h"
#include "SurToolAux.h"
#include "IVisGeneric.h"

// CSurToolColorPic dialog

class CSurToolColorPic : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolColorPic)

public:
	CSurToolColorPic(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolColorPic();

	CEScroll m_CenterAlpha;
	int state_RButton_DrawErase;
	CEScroll m_FilterMinH;
	CEScroll m_FilterMaxH;
	static int filterMinHValue;
	static int filterMaxHValue;
	static bool flag_EnableFilterH;


	Vect2i previewTextureSize_;
	cTexture* previewTexture_;
// virtuals
	void CallBack_CreateScene(void);
	void CallBack_ReleaseScene(void);
	bool CallBack_OperationOnMap(int x, int y);
	bool CallBack_DrawAuxData(void);
	bool CallBack_DrawPreview(int, int);
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

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnBrowseBitmap();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedRbutDrawerase1();
	afx_msg void OnBnClickedRbutDrawerase2();
	afx_msg void OnBnClickedCheckEnableHFilter();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};


#endif //__SURTOOLCOLORPIC_H__
