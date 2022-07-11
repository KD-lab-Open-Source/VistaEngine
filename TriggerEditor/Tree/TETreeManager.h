#ifndef __TE_TREE_MANAGER_H_INCLUDED__
#define __TE_TREE_MANAGER_H_INCLUDED__

#include <boost/scoped_ptr.hpp>
#include <prof-uis.h>

class TETreeDlg;
class TETreeNotifyListener;
class xTreeListCtrl;

class TETreeManager
{
public:
	enum eShowHide {SH_HIDE, SH_SHOW};

	TETreeManager(void);
	~TETreeManager(void);

	bool create(CFrameWnd* parentFrame_, 
				DWORD dwStyle = WS_CHILD |CBRS_FLYBY | 
				CBRS_RIGHT | CBRS_SIZE_DYNAMIC);
	CExtControlBar& controlBar(){ return controlBar_; }
    void enableDocking(bool enable);
	void dock(UINT dockBarID);
	/// Показать/спрятать
	bool show(eShowHide e);
	/// Видимо или нет
	bool isVisible() const;
	/// Реализация дерева
	xTreeListCtrl& getTreeListCtrl() const;
	/// Устанавливает обработчик событий
	void setTETreeNotifyListener(TETreeNotifyListener* ptl);
    
private:
	boost::scoped_ptr<TETreeDlg> treeDlg_;
	CExtControlBar controlBar_;
	CFrameWnd* parentFrame_;
};

#endif
