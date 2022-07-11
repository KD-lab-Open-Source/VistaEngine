#ifndef __FILE_LIST_H_INCLUDED__
#define __FILE_LIST_H_INCLUDED__

#include "kdw/Space.h"

namespace kdw{
	class Tree;
	class Label;
	class Box;
	class ComboBox;
}

class AnimationGroupComboBox;

class ChainsSpace : public kdw::Space
{
public:
	ChainsSpace();

	void onModelChanged();
	void onLODChanged();
protected:
	AnimationGroupComboBox* chainsComboBox_;
	kdw::Box* visibilityGroupsBox_;
	kdw::Box* animationGroupsBox_;
	kdw::ComboBox* lodCombo_;
};

#endif
