#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

#include "Handle.h"
#include "LibraryWrapper.h"
#include "Timers.h"

#include "UI_Types.h"

class UI_GlobalAttributes : public LibraryWrapper<UI_GlobalAttributes>
{
public:
	UI_GlobalAttributes();

	void serialize(Archive& ar);

	const UI_Cursor* cursor(UI_CursorType cur_type) const;
	const UI_Cursor* getMoveCursor(int dir);
	void setCursor(UI_CursorType cur_type, const UI_Cursor* cursor);

	float chatDelay() const { return chatDelay_; }

	const UI_MessageSetup& messageSetup(int index) const{ return messageSetups_[index]; }
	const sColor4f& privateMessage() const{ return privateMessage_; }
	const sColor4f& systemMessage() const{ return systemMessage_; }
private:
	/// ����� ����������� ���-������ � ����
	float chatDelay_;
	/// �������
	typedef vector<UI_CursorReference> CursorVector;
	CursorVector cursors_;
	/// ���� ������� ��������� � ����
	sColor4f privateMessage_;
	/// ���� ���������� ��������� � ����
	sColor4f systemMessage_;

	typedef std::vector<UI_MessageSetup> MessageSetups;
	/// ��������������� ���������, ������� �� UI_MessageID
	MessageSetups messageSetups_;
};

class UI_Dispatcher : public LibraryWrapper<UI_Dispatcher>
{
public:
	UI_Dispatcher();
	~UI_Dispatcher();

	void reset();
	void resetScreens();
	void relaxLoading();

	void serializeUIEditorPrm(Archive& ar);
	void serializeUserSave(Archive& ar);
	void serialize(Archive& ar);

	bool isEnabled() const { return isEnabled_; }
	void setEnabled(bool isEnabled);

	bool canExit() const;

	bool isModalMessageMode() const { return isModalScreen_ && messageScreen_; }
	const char* getModalMessage() const { return messageBoxMessage_.c_str(); }

	void exitMissionPrepare();
	void exitGamePrepare();

	void init();
	void quant(float dt);
	void redraw() const;
	void quickRedraw();

	void setFocus(UI_ControlBase* control);

	void logicQuant(bool pause = false);

	void messageBox(const char* message);
	bool specialExitProcess();
	void closeMessageBox();

	bool releaseResources();

	/// ���������� ����� ����� ������
	float getSelectScreenTime(const UI_Screen* scr) const;
	/// ������������� �����, ��������� ��������� ������ ���� ������ ���, ��� ������������
	void preloadScreen(UI_Screen* scr, cScene* scene = 0, const Player* player = 0);
	/// ����������� ������� �����
	void selectScreen(UI_Screen* scr);
	void setLoadingScreen();
	bool isScreenActive(const UI_Screen* screen) const { return screen == currentScreen_; }

	void convertInputEvent(UI_InputEvent& event);
	/// ��������� ����� � ���������� ��� ����
	/**
	���������� true ���� ������� ���� ����������
	*/
	bool handleInput(const UI_InputEvent& event);

	/// ������������ � �������� ������ �������������� ������� ��������� ������
	void handleMessage(const ControlMessage& msg);

	/// ����� ������ �� �����
	UI_Screen* screen(const char* screen_name);
	UI_Screen* screen(UI_Screen::ScreenType type_);
	const UI_Screen* currentScreen() const { return currentScreen_; }

	bool addScreen (const char* name);
	bool removeScreen (const char* name);

	// ��� UIEditor-�:
	typedef std::list<UI_Screen> ScreenContainer;
    ScreenContainer& screens () { return screens_; }

	const char* screenComboList() const { return screenComboList_.c_str(); }

	void updateControlOwners();
	bool controlComboListBuild(std::string& list_string, UI_ControlFilterFunc filter = 0) const;

	float tipsDelay() const { return tipsDelay_; }

	bool hasFocusedControl() const { return focusedControl_ != 0; }

	/// �������

	/// ������������ �����
	void initBgScene();
	void finitBgScene();

	void lockUI() { lockUISection_.lock(); }
	void unlockUI() { lockUISection_.unlock(); }
	MTSection& getLock() { return lockUISection_; }

	void showDebugInfo() const; //MTL
	void drawDebugInfo() const; //MTG 3D
	// ��������� ���������� ���������
	void drawDebug2D() const;
	void updateDebugControl();

	/// ������ � ���������

	void setTask(UI_TaskStateID state, const UI_MessageSetup& message_setup, bool is_secondary = false);
	bool getTaskList(std::string& str, bool reverse = false) const;

	bool sendMessage(UI_MessageID id);
	bool sendMessage(const UI_MessageSetup& message_setup, const char* custom_text = 0);
	bool getMessageQueue(std::string& str, const UI_MessageTypeReferences& filter, bool reverse = false) const;

	bool removeMessage(const UI_MessageSetup& message_setup, bool independent = false);
	bool playVoice(const UI_MessageSetup& setup);

	bool isIngameHotKey(const sKey& key) const;

	void clearTexts(){ tasks_.clear(); clearMessageQueue(); }

	int getNextControlID() { return controlID_++; }

	const char* privateMessageColor() const { return privateMessageColor_; }
	const char* systemMessageColor() const { return systemMessageColor_; }

	bool isActive(const UI_Screen* scr) const { return scr == currentScreen_ || scr == graphCurrentScreen_; }

	bool autoConfirmDiskOp() const { return autoConfirmDiskOp_; }

	int gameMajorVersion() const { return 1; }
	int gameMinorVersion() const { return 1; }

private:

	UI_Screen* currentScreen_;
	/// ��������� �� ������� ����� ��� ��������� �� ������������ ������
	UI_Screen* graphCurrentScreen_;
	/// ����� ��� ������ ��������� ���������
	UI_Screen* messageScreen_;
	/// ���������� ���������� ���������
	MTSection lockUISection_;

	bool isEnabled_;
	
	bool isModalScreen_;

	bool exitGameInProgress_;
	bool needResetScreens_;

	int controlID_;

	ScreenContainer screens_;
	std::string screenComboList_;

	UI_ControlBase* focusedControl_;

	/// �������� ��������� ���������
	float tipsDelay_;

	/// ��������� ���������� ������/�������/��������� ���������
	bool autoConfirmDiskOp_;

	typedef std::list<UI_Message> MessageQueue;
	/// ������� ���������, ������ ��������� �� �����
	MessageQueue messageQueue_;
	/// ���� ������� ������ ��������� ��������� ���������
	DurationTimer voiceLockTimer_;

	char privateMessageColor_[8];
	char systemMessageColor_[8];

	/// �����, ������� ������ ���� ������ ���������, ������ ��� ��������� ������������ ������
	UI_Screen* preloadedScreen_;
	/// ����� ������� ����� (����� ����������� �������� ������)
	bool needChangeScreen_;
	/// �����, ������� ��������������� ����� ����������� �������� ������
	UI_ScreenReference nextScreen_;

	/// ������� ��������� ������ �����
	bool currentClickAttackState_;
	/// ���������� ������� �� ������� ������� ���������
	unsigned short attackCoordTime_;

	typedef std::list<UI_Task> UI_Tasks;
	/// ������
	UI_Tasks tasks_;

	typedef std::vector<sKey> KeyList;
	KeyList ingameHotKeyList_;

	/// ��������� ��� ���������� �������
	string messageBoxMessage_;


	void screenComboListUpdate();
	void parseMarkScreens();

	void setCurrentScreenLogic(UI_Screen* screen);
	void setCurrentScreenGraph(UI_Screen* screen);

	friend void fCommandSetCurrentScreenGraph(void* data);

	void clearMessageQueue(bool full = false); 

	void graphQuantCommit() { InterlockedExchange(&graphAfterLogicQuantSequence, 1); }
	volatile long graphAfterLogicQuantSequence;

	int debugLogQuant_;
	int debugGraphQuant_;
	const UI_ControlBase* debugControl_;
	friend void logUIState(int line, const char* func, const UI_ControlBase* control, const char* str);
};

// ��� ��������� � ������� �� ������� ��� ���� �����
#define UI_AUTOLOCK() MTAuto ui_autolock(UI_Dispatcher::instance().getLock())

#endif /* __USER_INTERFACE_H__ */
