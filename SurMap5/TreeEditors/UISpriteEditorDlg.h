#ifndef __U_I_SPRITE_EDITOR_DLG_H_INCLUDED__
#define __U_I_SPRITE_EDITOR_DLG_H_INCLUDED__

#include "Handle.h"

class CSizeLayoutManager;
class CSolidColorCtrl;
class CUISpriteEditor;

class UI_Sprite;

class CUISpriteEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CUISpriteEditorDlg)

public:
	CUISpriteEditorDlg(UI_Sprite& sprite, CWnd* pParent = NULL);   // standard constructor
	virtual ~CUISpriteEditorDlg();

    void UpdateControls ();
	void UpdateTexturesCombo ();

// Dialog Data
	enum { IDD = IDD_UI_SPRITE_EDITOR_DLG };

	enum { SATURATION_SLIDER_MAX_VALUE = 10 };
	afx_msg void OnPrevFrameButton();
	afx_msg void OnNextFrameButton();
	afx_msg void OnPlayCheck();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTexturesComboSelChange();
	afx_msg void OnCoordChange();
	afx_msg void OnTextureLocAddButtonClicked();
	afx_msg void OnTextureAddButtonClicked();
	afx_msg void OnTextureRemoveButtonClicked();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    // virtuals:
	BOOL OnInitDialog();
protected:
	void onDiffuseColorChanged();
	void onBackgroundColorChanged();

	CSliderCtrl saturationSlider_;
	CEdit m_ctlCoordLeftEdit;
	CEdit m_ctlCoordTopEdit;
	CEdit m_ctlCoordWidthEdit;
	CEdit m_ctlCoordHeightEdit;
	CStatic m_ctlTextureSizeLabel;
	CComboBox m_ctlTexturesCombo;


	PtrHandle<CUISpriteEditor> editor_;
	PtrHandle<CSizeLayoutManager> layout_;
	
	PtrHandle<CSolidColorCtrl> diffuseColorSelector_;
	PtrHandle<CSolidColorCtrl> backgroundColorSelector_;

	UI_Sprite& sprite_;


    // virtuals:
	void DoDataExchange(CDataExchange* pDX);
	void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

#endif
