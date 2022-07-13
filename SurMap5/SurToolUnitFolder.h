#ifndef __SUR_TOOL_UNIT_FOLDER_H_INCLUDED__
#define __SUR_TOOL_UNIT_FOLDER_H_INCLUDED__

#include "SurToolAux.h"
#include "SurToolEmpty.h"

// CSurToolUnitFolder dialog

class CSurToolUnitFolder : public CSurToolEmpty
{
	DECLARE_DYNAMIC(CSurToolUnitFolder)

public:
	CSurToolUnitFolder(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolUnitFolder();
	virtual BOOL OnInitDialog ();
	virtual void FillIn ();

	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_UNITFOLDER; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};

#endif
