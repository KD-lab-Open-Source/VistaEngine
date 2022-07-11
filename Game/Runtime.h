#ifndef __RUNTIME_H__
#define __RUNTIME_H__

class MissionDescription;

class Runtime
{
public:
	Runtime(HINSTANCE hInstance, bool ht);
	virtual ~Runtime();

	void init();
	void done();

	void createScene();
	void destroyScene();

	void restoreFocus();
	void updateResolution(Vect2i size, bool change_size);
	void updateDefaultFont();
	
	virtual void eventHandler(UINT uMsg,WPARAM wParam,LPARAM lParam) {}

	void updateWindowSize();
	const Vect2i& windowClientSize() const { return windowClientSize_; }

	bool alwaysRun() const { return alwaysRun_ || load_mode; }
	bool applicationRuns() const { return applicationHasFocus() || (alwaysRun() && !ErrH.IsErrorOrAssertHandling()); }

	virtual void onSetFocus(bool focus);
	virtual void onSetCursor(){}
	virtual void onClose(){}
	virtual void onAbort(){}

	bool quant();

	virtual void graphicsQuant(){}
	virtual void logicQuantST(){}
	virtual void logicQuantHT(){}

	void GameStart(const MissionDescription& mission);
	virtual void GameClose();

	bool useHT()const { return useHT_; }
	bool PossibilityHT();

	HWND hWnd() { return hWnd_; }

	bool terminateLogicThread() const { return end_logic != 0; }

	static Runtime* instance() { return instance_; }

protected:
	HINSTANCE hInstance_;
	HWND hWnd_;
	bool useHT_;
	bool alwaysRun_;
	bool GameContinue;
	bool init_logic;
	bool load_mode;
	HANDLE load_finish;
	HANDLE end_logic;
	DWORD logic_thread_id;

	Vect2i windowClientSize_;

	static const DWORD bad_thread_id;

	static Runtime* instance_;
	static bool applicationHasFocus_;

	void logic_thread(const MissionDescription*);

	void checkSingleRunning();

	HWND createWindow(const char* title, const char* icon, int xPos,int yPos,int xScr,int yScr,WNDPROC lpfnWndProc,int dwStyle);
	void calcRealWindowPos(int xPos,int yPos,int xScr,int yScr,bool fullscreen, unsigned int windowStyle, Vect2i& pos,Vect2i& size);
	void repositionWindow(Vect2i size);
	void setWindowPicture(const char* file);

	friend void logic_thread( void * argument);
	friend bool applicationHasFocus();

private:
	virtual void GameLoad(const MissionDescription& mission) = 0;
	virtual void GameRelaxLoading() = 0;
};

bool applicationHasFocus();

Runtime* createRuntime(HINSTANCE hInstance);

extern const char* currentVersion;

#endif //__RUNTIME_H__
