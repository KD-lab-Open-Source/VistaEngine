#pragma once

#include "kdw/Viewport2D.h"
#include "kdw/Tooltip.h"
#include "TriggerEditor\TriggerExport.h"
#include "Serialization\BinaryArchive.h"

class TriggerMiniMap;

namespace kdw {
	class PropertyTree;
};


class TriggerView : public kdw::Viewport2D
{
public:
	TriggerView(TriggerChain& triggerChain, TriggerMiniMap* miniMap, kdw::PropertyTree* propertyTree);
	void setMiniMap(TriggerMiniMap* miniMap) { miniMap_ = miniMap; } 

	void setDebug(bool debug) { debug_ = debug; }

	void onMouseMove(const Vect2i& delta);
	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);

	void onKeyDown(const sKey& key);

	void onRedraw(HDC dc);
//	void trackMouse(const Vect2f& point, bool dragOver = false);

	void undo();
	void redo();
	void find();

	bool changed() const { return history_.size() > 1; }
	
	void showLegend();

private:
	TriggerChain& triggerChain_;
	typedef UniqueVector<int> Indices;
	Indices selectedTriggers_;
	TriggerLink* selectedLink_;

	bool debug_;
	bool moving_;
	bool movingParent_;
	Vect2i movingDelta_;
	bool deselectWhenKeyUp_;

	bool areaSelection_;
	Vect2f clickPoint_;
	Rectf areaRectangle_;

	bool creatingLink_;
	bool wasMouseMove_;
	bool popupMenu_;
	bool find_;
	int findIndex_;
	bool legend_;

	Color4c triggerColor_;
	int linkColor_;
	bool linkAutoRestarted_;

	Color4c triggerColorPrev_;
	int linkColorPrev_;
	bool linkAutoRestartedPrev_;

	string triggerName_;

	kdw::Tooltip tooltip_;
	TriggerMiniMap* miniMap_;
	kdw::PropertyTree* propertyTree_;

	enum { HISTORY_STEPS = 20 };
	typedef vector<ShareHandle<BinaryOArchive> > History;
	History history_;
	int undoIndex_;

	static TriggerChain copiedTriggers_;

	void drawLinks(HDC dc, Trigger& trigger);
	void drawTrigger(HDC dc, Trigger& trigger, bool moved);

	int findTrigger(const Vect2f& point);
	TriggerLink* findLink(const Vect2f& point);
    void deselect();
	void selectArea(const Rectf& rect);
	bool checkToMove();
	void moveTriggers();

	void copyTriggers();
	void pasteTriggers();
	
	void deleteTriggers();
	void deleteLink();
	void createTrigger(int index);
	void createLink();
	void editConditions();
	void editAction();
	void editTrigger();
	
	void popupMenu();
	void treeSelect();
	void treeSelectRecursive(Trigger& trigger);
	void updatePropertyTree();
	void onPropertyChanged();

	void insertRow();
	void insertColumn();
	void deleteRow();
	void deleteColumn();

	void saveStep();
};
