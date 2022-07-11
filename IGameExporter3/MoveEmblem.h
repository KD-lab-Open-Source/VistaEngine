#pragma once

#include "..\Render\inc\umath.h"
char* GetFileName(const char *FullName);

struct sLogo
{
	sLogo(){angle = 0;rect.set(0,0,0,0);};
	sLogo(const char* name)
	{
		enabled = false;
		TextureName = name;
		angle = 0;
		rect.set(0,0,0,0);
	}
	bool enabled;
	sRectangle4f rect;
	float angle;
	string TextureName;
};

class cMoveEmblem;

class cMyPicture : public CStatic
{
	DECLARE_DYNAMIC(cMyPicture)

public:
	cMyPicture();
	virtual ~cMyPicture();
protected:
	DECLARE_MESSAGE_MAP()
public:
	
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	cMoveEmblem* parentDlg;
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


// cMoveEmblem dialog
class cMoveEmblem : public CDialog
{
	DECLARE_DYNAMIC(cMoveEmblem)

public:
	cMoveEmblem(CWnd* pParent = NULL);   // standard constructor
	virtual ~cMoveEmblem();
	vector<sLogo> logos_;
	int GetCurrentMaterial(const char* fname);
// Dialog Data
	enum { IDD = IDD_MOVE_EMBLEM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	cMyPicture m_DrawTexture;
	virtual BOOL OnInitDialog();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	CListCtrl m_MaterialsList;
	afx_msg void OnLvnItemchangedMaterialsList(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	string texturePath;
public:
	afx_msg void OnOk();
	CScrollBar m_hScroll;
	CScrollBar m_vScroll;
	afx_msg void OnBnClickedZoomin();
	afx_msg void OnBnClickedZoomout();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedOneone();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	CString m_Width;
	CString m_Height;
	afx_msg void OnBnClickedBok();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedColor();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	BOOL m_EnableLogo;
	afx_msg void OnBnClickedUseLogo();
	CString m_PosX;
	CString m_PosY;
	CString m_Angle;
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnBnClickedButtonSelectTexturePath();
	CString m_TexturePath;
	afx_msg void OnBnClickedButtonCopy();
	afx_msg void OnBnClickedButtonPaste();
};
