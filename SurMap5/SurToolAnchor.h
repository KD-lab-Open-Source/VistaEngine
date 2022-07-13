#ifndef __SUR_TOOL_ANCHOR_H_INCLUDED__
#define __SUR_TOOL_ANCHOR_H_INCLUDED__

#include "SurToolEditable.h"

#include "EventListeners.h"
#include "Environment\Environment.h"

class Anchor;

class Archive;
class CSurToolAnchor : public CSurToolEditable, public ObjectObserver{
	DECLARE_DYNAMIC(CSurToolAnchor)
public:
	CSurToolAnchor(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolAnchor();

	bool onOperationOnMap(int x, int y);
	void onBrushRadiusChanged();
	bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoord);

	void onPropertyChanged();

	bool onDrawAuxData(void);

	void serialize(Archive& ar);
	const Anchor* originalAnchor();

	static Anchor* anchorOnMouse();
    static void setAnchorOnMouse(Anchor* anchor);

    static void killAnchor();
private:
	ShareHandle<Anchor> anchor_;
	static Anchor* anchorOnMouse_;

	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
};

#endif
