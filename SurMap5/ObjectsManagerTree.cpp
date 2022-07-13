#include "StdAfx.h"
#include "ObjectsManagerTree.h"
#include "ObjectsManagerWindow.h"

CObjectsManagerTree::CObjectsManagerTree(CObjectsManagerWindow* window)
: window_(window)
{
}

void CObjectsManagerTree::onRightClick()
{
	window_->onObjectsTreeRightClick();
}
