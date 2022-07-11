#ifndef __PROPERTY_SPACE_H_INCLUDED__
#define __PROPERTY_SPACE_H_INCLUDED__

#include "kdw/Space.h"
#include "kdw/Document.h"

namespace kdw{
	class PropertyTree;
	class ModelObserver;
	class ModelNode;
}

class PropertySpace : public kdw::Space, kdw::ModelObserver{
public:
	PropertySpace();

	void onModelChanged(kdw::ModelObserver* changer);
	void onCurveChanged(bool significantChange);
	void onPropertyChanged();
	void onPropertyBeforeChange();
protected:
	kdw::PropertyTree* tree_;
	kdw::ModelNode* attachedNode_;
};

#endif
