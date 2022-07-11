#ifndef __SCALE_MGR_H_INCLUDED__
#define __SCALE_MGR_H_INCLUDED__

#include "scaleinterfaces.h"
#include <memory>

class CScaleBar;
class ScaleMgr :
	public IScaleMgr
{
public:
	void SetScalable(IScalable* pscalable);
	//обновляем информацию о масштабе
	void UpdateScaleInfo();

	ScaleMgr(void);
	~ScaleMgr(void);

	bool Create(CFrameWnd* pParent, DWORD barid);
	void Show() const;
	void Hide() const;
	bool IsVisible() const;

	//возвращает указатель на окно
	CWnd* getWindow();
	CScaleBar& controlBar();

	void enableDocking(bool enable);
	void dock(UINT doCSckBarID);
private:
	CScaleBar& scaleBar_;
	CFrameWnd* parentFrame_;
};

#endif
