#ifndef __SUR_TOOL_SOURCE_H_INCLUDED__
#define __SUR_TOOL_SOURCE_H_INCLUDED__

#include "SurToolEditable.h"
#include "Environment\SourceBase.h"
#include "EventListeners.h"

class Archive;
struct SourceReferenceHolder;
class CSurToolSource : public CSurToolEditable, public ObjectObserver {

	DECLARE_DYNAMIC(CSurToolSource)
public:
	CSurToolSource(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolSource();

	void quant();

	bool onOperationOnMap(int x, int y);
	void onBrushRadiusChanged();
	bool onTrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoord);

	void onPropertyChanged();

	bool onDrawAuxData(void);

	void serialize(Archive& ar);
	const SourceBase* originalSource();

	static SourceBase* sourceOnMouse() {
		return sourceOnMouse_;
	}
	static void setSourceOnMouse(SourceBase* source) {
		sourceOnMouse_ = source;
	}

private:
	ShareHandle<SourceBase> source_;
	bool sourceActive_;

	PtrHandle<SourceReferenceHolder> refHolder_;
	static class SourceBase* sourceOnMouse_;

	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
};

#endif
