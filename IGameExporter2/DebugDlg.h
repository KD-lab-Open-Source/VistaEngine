#pragma once

extern bool dbg_show_position;
extern bool dbg_show_rotation;
extern bool dbg_show_scale;
extern bool dbg_show_info_polygon;
extern bool dbg_show_info_delete_node;


class CDebugDlg : public CDialog
{
	DECLARE_DYNAMIC(CDebugDlg)

public:
	CDebugDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDebugDlg();

// Dialog Data
	enum { IDD = IDD_DEBUG_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
public:
	BOOL m_bRotate;
	BOOL m_bPosition;
	BOOL m_bScale;
	BOOL m_bPolygon;
	BOOL m_bDelNode;
};
