#ifndef __TOOL_TIP_H_INCLUDED__
#define __TOOL_TIP_H_INCLUDED__

class CWnd;

class ToolTip  
{
public:
	ToolTip();
	virtual ~ToolTip();
	CString const& getToolTipText() const;
	//! Назначает текст тултипа
	void setToolTipText(CString const& str) const;
	//! показывает тултип
	void trackToolTip(CPoint const& pos) const;
	//! создание тултипа
	HWND create(HWND owner);
	void setOwnerWindow(HWND owner);
	HWND getOnwenerWindow() const;

	//! Вызывать из OnTTNGetDispInfo окна
	BOOL OnTTNGetDispInfo(UINT id, NMHDR * pTTTStruct, LRESULT * pResult ) const;
private:
	//! Текст туллтипа
	mutable CString toolTipText_;
	//! Окно тултипа
	HWND toolTipWindow_;
	//! Окно владельца
	HWND ownerWindow_;
};

#endif
