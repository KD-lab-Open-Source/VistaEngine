#ifndef __SUR_TOOL_SELECT_H_INCLUDED__
#define __SUR_TOOL_SELECT_H_INCLUDED__

#include "SurToolAux.h"
#include "..\Util\MFC\SizeLayoutManager.h"
#include "Serializeable.h"
#include "EventListeners.h"

class CAttribEditorCtrl;

class CSurToolSelect : public CSurToolBase,
	public ObjectChangedListener, public SelectionChangedListener
{
	DECLARE_DYNAMIC(CSurToolSelect)
public:
	struct SelectionCount{
		SelectionCount()
		: numUnits(0)
		, numSources(0)
		, numEnvironment(0)
		, numAnchors(0)
		, numCameras(0)
		{
		}
		int numUnits;
		int numSources;
		int numEnvironment;
		int numAnchors;
		int numCameras;
	};

	typedef std::vector<Serializeable> Serializeables;

	CSurToolSelect(CWnd* pParent = NULL);
	~CSurToolSelect();

    void serialize(Archive& ar);

	bool CallBack_TrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool CallBack_RMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	void CallBack_SelectionChanged();

	bool CallBack_Delete();
	bool CallBack_KeyDown (unsigned int keyCode, bool shift, bool control, bool alt);

	bool CallBack_DrawAuxData(void);
	void updateLayout();
	void showControls(bool multiselect);

	int getIDD() const { return IDD_BARDLG_SELECT; }

	void onObjectChanged();
	void onSelectionChanged();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEditButton();
	afx_msg void OnRandomSelectionClicked();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
private:
	void calcSubEditorRect(CRect& rect);

	Serializeables serializeables_;
	std::string selectionDescription_;
	SelectionCount selectionCount_;

    CSizeLayoutManager layout_;
    PtrHandle<CAttribEditorCtrl> attribEditor_;

	bool isSelectionEditable_;
	bool isSelectionMovable_;

	bool localTransform_;
	Vect2i cursorCoord_;
	Vect3f endPoint_;

	Vect3f selectionStart_;
	Vect3f selectionCenter_;
	float selectionRadius_;
    float randomSelectionChance_;
    
	bool flag_mouseLBDown;

	static std::vector<CSurToolSelect*> instances_;
public:
	afx_msg void OnRandomSelPercentChange();
};

#endif
