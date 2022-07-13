#ifndef __SUR_TOOL_MINIDETAILE_FOLDER_H_INCLUDED__
#define __SUR_TOOL_MINIDETAILE_FOLDER_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolEmpty.h"

class CSurToolMiniDetaileFolder : public CSurToolEmpty
{
	DECLARE_DYNAMIC(CSurToolMiniDetaileFolder)

public:
	CSurToolMiniDetaileFolder(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolMiniDetaileFolder();
	virtual BOOL OnInitDialog ();
	virtual void FillIn ();

	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_MINIDETAILE_FOLDER; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};


#endif //__SUR_TOOL_MINIDETAILE_FOLDER_H_INCLUDED__