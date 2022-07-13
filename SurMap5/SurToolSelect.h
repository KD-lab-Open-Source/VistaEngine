#ifndef __SUR_TOOL_SELECT_H_INCLUDED__
#define __SUR_TOOL_SELECT_H_INCLUDED__

#include "SurToolAux.h"
#include "MFC\SizeLayoutManager.h"
#include "Serialization\Serializer.h"
#include "EventListeners.h"

class CAttribEditorCtrl;

class CSurToolSelect : public CSurToolBase, public sigslot::has_slots,
	public ObjectObserver, public SelectionObserver
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

	typedef std::vector<Serializer> Serializers;

	CSurToolSelect(CWnd* pParent = NULL);
	~CSurToolSelect();

    void serialize(Archive& ar);

	bool onTrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);
	bool onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord);
	bool onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord);
	void onSelectionChanged();

	bool onDelete();
	bool onKeyDown (unsigned int keyCode, bool shift, bool control, bool alt);

	bool onDrawAuxData(void);
	void updateLayout();
	void showControls(bool multiselect);

	int getIDD() const { return IDD_BARDLG_SELECT; }

	void onObjectChanged(ObjectObserver* changer);
	void onSelectionChanged(SelectionObserver* changer);
	void onPropertyChanged();

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

	Serializers serializeables_;
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
	bool objectChangedEmitted_;

	static std::vector<CSurToolSelect*> instances_;
public:
	afx_msg void OnRandomSelPercentChange();
};

#endif
