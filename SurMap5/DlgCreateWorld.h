#ifndef __DLG_CREATE_WORLD_H_INCLUDED__
#define __DLG_CREATE_WORLD_H_INCLUDED__

#include "Serialization.h"
#include "..\AttribEditor\AttribEditorCtrl.h"
#include "..\Render\inc\TerraInterface.inl"

// CDlgCreateWorld dialog

class CDlgCreateWorld : public CDialog
{
	DECLARE_DYNAMIC(CDlgCreateWorld)

public:
	CDlgCreateWorld(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCreateWorld();

// Dialog Data
	enum { IDD = IDD_DLG_CREATEWORLD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	vrtMapCreationParam m_CreationParam;
protected:
	CAttribEditorCtrl m_ctlAttribEditor;
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
public:
	CString m_strWorldName;
};

#endif
