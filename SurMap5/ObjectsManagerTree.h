#ifndef __OBJECTS_MANAGER_TREE_H_INCLUDED__
#define __OBJECTS_MANAGER_TREE_H_INCLUDED__

#include "mfc\ObjectsTreeCtrl.h"

class CObjectsManagerWindow;
class CObjectsManagerTree: public CObjectsTreeCtrl{
public:
	CObjectsManagerTree(CObjectsManagerWindow* window);

	// virtuals:
	void onRightClick();
	// ^^^
protected:
	CObjectsManagerWindow* window_;
};

#endif
