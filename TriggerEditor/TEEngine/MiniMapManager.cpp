#include "stdafx.h"
#include <prof-uis.h>
#include "resource.h"
#include "TEMiniMap.h"
#include "MiniMapManager.h"

TEMiniMapManager::TEMiniMapManager(TriggerEditor* editor)
: parentFrame_(0)
, controlBar_(*new CExtControlBar())
, miniMap_(*new TEMiniMap(editor))
{
}

TEMiniMapManager::~TEMiniMapManager()
{
	delete &controlBar_;
    delete &miniMap_;
}

bool TEMiniMapManager::create(CFrameWnd* pParent)
{
	parentFrame_ = pParent;

	if (!controlBar_.Create("Minimap", parentFrame_, ID_VIEW_MINIMAP))
		return false;

	miniMap_.Create(&controlBar_);

	int height = 50;

	controlBar_.SetInitDesiredSizeVertical(CSize(250, height));
	controlBar_.SetInitDesiredSizeHorizontal(CSize(250, height));
	controlBar_.SetInitDesiredSizeFloating(CSize(250, height));
	return true;
}

void TEMiniMapManager::enableDocking(bool enable)
{
    if(enable)
        controlBar_.EnableDocking(CBRS_ALIGN_ANY);
    else
		controlBar_.EnableDocking(0);
}

void TEMiniMapManager::dock(UINT dockBarID)
{
	parentFrame_->DockControlBar(&controlBar_, dockBarID);
}

bool TEMiniMapManager::isVisible() const{
	return (IsWindowVisible(controlBar_.GetSafeHwnd()) == TRUE);
}

void TEMiniMapManager::show() const
{
	parentFrame_->ShowControlBar(&controlBar_, TRUE, FALSE); 
}

void TEMiniMapManager::hide() const
{
	parentFrame_->ShowControlBar(&controlBar_, FALSE, FALSE); 
}

//возвращает указатель на окно
CWnd* TEMiniMapManager::getWindow()
{
	return &controlBar_;
}

CExtControlBar& TEMiniMapManager::controlBar()
{
	return controlBar_;
}

void TEMiniMapManager::update()
{
    miniMap_.Invalidate(FALSE);
}

void TEMiniMapManager::setView(TriggerEditorView* view)
{
    miniMap_.setView(view);
}
