#ifndef __SUR_TOOL_PATH_EDITOR_H_INCLUDED__
#define __SUR_TOOL_PATH_EDITOR_H_INCLUDED__

#include "SurToolAux.h"
#include "..\Util\MFC\SizeLayoutManager.h"

class SourceBase;
class BaseUniverseObject;

class CSurToolPathEditor : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolPathEditor)
public:
	CSurToolPathEditor(BaseUniverseObject* source);   // standard constructor
	virtual ~CSurToolPathEditor();

	bool CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_RMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	void CallBack_SelectionChanged ();

	bool CallBack_Delete();
    bool CallBack_KeyDown(unsigned int keyCode, bool shift, bool control, bool alt);

	bool CallBack_DrawAuxData();

	int nodeUnderPoint(const Vect3f& worldCoord);
    void setSelectedNode(int node);
	int getIDD() const { return IDD_BARDLG_PATHEDITOR; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
private:
    CSizeLayoutManager layout_;
	SourceBase* source_;

	int selectedNode_;
	bool moving_;

public:
	afx_msg void OnSpeedEditChange();
	afx_msg void OnAddToLibraryButton();
};

#endif
