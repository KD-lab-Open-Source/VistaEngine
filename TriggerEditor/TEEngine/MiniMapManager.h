#ifndef __MINI_MAP_MANAGER_H_INCLUDED__
#define __MINI_MAP_MANAGER_H_INCLUDED__
//#include <memory>

class TEMiniMap;
class TriggerEditor;
class TriggerEditorView;
class CExtControlBar;

class TEMiniMapManager
{
public:
	TEMiniMapManager(TriggerEditor* editor);
	~TEMiniMapManager();

	bool create(CFrameWnd* pParent);
    void setView(TriggerEditorView* view);
	void show() const;
	void hide() const;
	bool isVisible() const;
    void update();

	//возвращает указатель на окно
	CWnd* getWindow();
	CExtControlBar& controlBar();

	void dock(UINT doCSckBarID);
    void enableDocking(bool enable);
private:
    TEMiniMap& miniMap_;
	CExtControlBar& controlBar_;
	CFrameWnd* parentFrame_;
};

#endif
