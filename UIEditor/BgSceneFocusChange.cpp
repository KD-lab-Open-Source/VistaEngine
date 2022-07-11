#include "StdAfx.h"

#include "BgSceneFocusChange.h"
#include "UserInterface\UI_BackgroundScene.h"

void BgFocusChangeAction::act()
{
	UI_BackgroundScene::instance().setFocus(newFocus_);
}

void BgFocusChangeAction::undo()
{
	UI_BackgroundScene::instance().setFocus(oldFocus_);
}
