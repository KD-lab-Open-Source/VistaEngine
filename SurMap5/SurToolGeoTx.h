#ifndef __SURTOOLGEOTX_H__
#define __SURTOOLGEOTX_H__

#include "SurToolAux.h"

// CSurToolGeoTx dialog

class CSurToolGeoTx : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolGeoTx)

public:
	CSurToolGeoTx(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolGeoTx();

	virtual bool onOperationOnMap(int x, int y);

	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_GEOTX; }
	string dataFileName2;
private:
	bool flag_changedGeoViewOption;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnBrowseFile();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnBrowseFile2();
	afx_msg void OnDestroy();
};

#endif //__SURTOOLGEOTX_H__
