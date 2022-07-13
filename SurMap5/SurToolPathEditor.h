#ifndef __SUR_TOOL_PATH_EDITOR_H_INCLUDED__
#define __SUR_TOOL_PATH_EDITOR_H_INCLUDED__

#include "SurToolAux.h"
#include "MFC\SizeLayoutManager.h"

class SourceBase;
class BaseUniverseObject;

class CSurToolPathEditor : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolPathEditor)
public:
	CSurToolPathEditor(BaseUniverseObject* source);   // standard constructor
	virtual ~CSurToolPathEditor();

	bool onTrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	void onSelectionChanged ();

	bool onDelete();
    bool onKeyDown(unsigned int keyCode, bool shift, bool control, bool alt);

	bool onDrawAuxData();

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
