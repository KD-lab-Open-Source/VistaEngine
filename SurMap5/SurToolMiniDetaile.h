#ifndef __SUR_TOOL_MINI_DETAILE_H_INCLUDED__
#define __SUR_TOOL_MINI_DETAILE_H_INCLUDED__

#include "SurToolAux.h"
#include "Resource.h"

class cTexture;
class CSurToolMiniDetail : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolMiniDetail)

	//static int next_layer;
	int layer;
	cTexture* previewTexture_;
	void setPreviewTexture(const char* tgaFName, const char* ddsFName);
public:
	CSurToolMiniDetail(int detailTextureLayer=0, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolMiniDetail();

	bool onDrawPreview(int width, int height);
	bool onOperationOnMap(int x, int y);
	bool onDrawAuxData();
	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_DETAILE; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnBrowseFile();
	afx_msg void OnBnClickedMiniDetailErace();
//	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnBnClickedDetailEraceAll();
};

#endif
