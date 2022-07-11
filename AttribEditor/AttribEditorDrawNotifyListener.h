#ifndef __ATTRIB_EDITOR_DRAW_NOTIFY_LISTENER_H_INCLUDED__
#define __ATTRIB_EDITOR_DRAW_NOTIFY_LISTENER_H_INCLUDED__

#include "..\Util\MFC\TreeListCtrl.h"
//#include "TreeListCtrl\ICustomDrawNotifyListener.h"

class CAttribEditorCtrl;

class AttribEditorDrawNotifyListener : public ICustomDrawNotifyListener{
public:
	AttribEditorDrawNotifyListener(CAttribEditorCtrl& ctrl);
	~AttribEditorDrawNotifyListener();

    // virtual:
	DWORD onPrepaint(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi);
	DWORD onPostpaint(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi);
	DWORD onPreerase(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi);
	DWORD onPosterase(CTreeListCtrl& source, CONTROL_CUSTOM_DRAW_INFO* pcdi);

	DWORD onItemPrepaint(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi);
	DWORD onItemPostpaint(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi);
	DWORD onItemPreerase(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi);
	DWORD onItemPosterase(CTreeListCtrl& source, ITEM_CUSTOM_DRAW_INFO* pcdi);

	DWORD onSubitemPrepaint(CTreeListCtrl& source, SUBITEM_CUSTOM_DRAW_INFO* pscdi);
	DWORD onSubitemPostpaint(CTreeListCtrl& source, SUBITEM_CUSTOM_DRAW_INFO* pscdi);
protected:
	CAttribEditorCtrl& attribEditor_;
};

#endif
