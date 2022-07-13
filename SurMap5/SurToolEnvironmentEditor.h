#ifndef __SUR_TOOL_ENVIRONMENT_EDITOR_H_INCLUDED__
#define __SUR_TOOL_ENVIRONMENT_EDITOR_H_INCLUDED__
#include "SurToolAux.h"
#include "..\Util\MFC\SizeLayoutManager.h"

class UnitEnvironment;
class BaseUniverseObject;

class CSurToolEnvironmentEditor : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolEnvironmentEditor)
public:
	CSurToolEnvironmentEditor(BaseUniverseObject* object);   // standard constructor
	virtual ~CSurToolEnvironmentEditor();

	int getIDD() const { return IDD_BARDLG_ENVIRONMENT_EDITOR; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
private:
    CSizeLayoutManager layout_;
	UnitEnvironment* unit_;
};

#endif
