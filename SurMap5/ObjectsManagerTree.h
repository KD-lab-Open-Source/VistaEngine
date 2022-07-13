#ifndef __OBJECTS_MANAGER_TREE_H_INCLUDED__
#define __OBJECTS_MANAGER_TREE_H_INCLUDED__

#include "mfc\ObjectsTreeCtrl.h"
#include "EventListeners.h"

class Player;
enum ObjectsManagerTab{
	TAB_SOURCES = 0,
	TAB_ENVIRONMENT,
	TAB_UNITS,
	TAB_CAMERA,
	TAB_ANCHORS,
    TAB_ALL
};

class CObjectsManagerTree;
class ObjectsManagerTree : public kdw::ObjectsTree, public SelectionObserver, public ObjectObserver{
public:
	ObjectsManagerTree();
	void onRowRMBDown(kdw::TreeRow* row, const Recti& rowRect, Vect2i point);
	bool onRowKeyDown(kdw::TreeRow* row, sKey key);
	void onRowSelected(kdw::TreeRow* row);
	bool onRowLMBDoubleClick(kdw::TreeRow* row, const Recti& rowRect, Vect2i point);
	void rebuild();
	void removeDead();
	
	void onMenuApplyPreset();
	void onMenuPaste();
	void onMenuDelete();
	void onMenuSortByName();
	void onMenuSortByTime();
	void onMenuChangePlayer(Player* player);
	bool sortByName()const { return sortByName_; }

	void selectOnWorld();
	//void selectObject(TreeObject* object, bool select = true);
	void updateSelectFromWorld();

	void setTab(ObjectsManagerTab tab);
protected:
	bool sortByName_;
	ObjectsManagerTab tab_;
};

class CObjectsManagerWindow;
class CObjectsManagerTree: public CObjectsTreeCtrl{
public:
	CObjectsManagerTree();	

	ObjectsManagerTree* tree(){ return safe_cast<ObjectsManagerTree*>(__super::tree()); }
};

#endif
