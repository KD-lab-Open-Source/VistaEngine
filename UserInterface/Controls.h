#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "MTSection.h"
#include "Serialization\LibraryWrapper.h"
#include "UserInterface\UI_Key.h"
#include "XTL\CallWrapper.h"

enum InterfaceGameControlID
{
	CTRL_CAMERA_MOVE_UP = 0,
	CTRL_CAMERA_MOVE_DOWN,
	CTRL_CAMERA_MOVE_LEFT,
	CTRL_CAMERA_MOVE_RIGHT,
	CTRL_CAMERA_ROTATE_UP,
	CTRL_CAMERA_ROTATE_DOWN,
	CTRL_CAMERA_ROTATE_LEFT,
	CTRL_CAMERA_ROTATE_RIGHT,
	CTRL_CAMERA_ZOOM_INC,
	CTRL_CAMERA_ZOOM_DEC,
	CTRL_CAMERA_MOUSE_LOOK,
	CTRL_DIRECT_UP,
	CTRL_DIRECT_DOWN,
	CTRL_DIRECT_LEFT,
	CTRL_DIRECT_RIGHT,
	CTRL_DIRECT_STRAFE_LEFT,
	CTRL_DIRECT_STRAFE_RIGHT,
	CTRL_PAUSE,
	CTRL_MAKESHOT,
	CTRL_SHOW_UNIT_PARAMETERS,
	CTRL_SHOW_ITEMS_HINT,
	CTRL_TOGGLE_MUSIC,
	CTRL_TOGGLE_SOUND,

	CTRL_CAMERA_TO_EVENT,
	
	CTRL_TIME_NORMAL,
	CTRL_TIME_DEC,
	CTRL_TIME_INC,

	CTRL_CLICK_ACTION,
	CTRL_ATTACK,
	CTRL_ATTACK_ALTERNATE,
	CTRL_SELECT,
	CTRL_SELECT_ALL,
	CTRL_MOVE,

	CTRL_MAX,
	
	CTRL_CAMERA_MAP_SHIFT,
	CTRL_LOAD,
	CTRL_SAVE,
	CTRL_CAMERA_RESTORE1,
	CTRL_CAMERA_RESTORE2,
	CTRL_CAMERA_RESTORE3,
	CTRL_CAMERA_RESTORE4,
	CTRL_CAMERA_RESTORE5,
	CTRL_CAMERA_SAVE1,
	CTRL_CAMERA_SAVE2,
	CTRL_CAMERA_SAVE3,
	CTRL_CAMERA_SAVE4,
	CTRL_CAMERA_SAVE5
};

class ControlManager : public LibraryWrapper<ControlManager>
{
public:

	ControlManager();

	void serialize(Archive& ar);

	InterfaceGameControlID control(int fullKey) const;
	const UI_Key& key(InterfaceGameControlID ctrl) const;
	UI_Key defaultKey(InterfaceGameControlID ctrl) const;

	bool isCompatible(InterfaceGameControlID ctrl1, InterfaceGameControlID ctrl2) const;

	void setKey(InterfaceGameControlID ctrl, const UI_Key& _key);

	void clearHotKeys();
	void registerHotKey(int fullKey, const CallWrapperBase* func);
	void unRegisterHotKey(const CallWrapperBase* func);
	bool checkHotKey(int fullKey);
	void handleKeypress(int fullKey);

private:
	struct ControlNode{
		ControlNode() : command_(CTRL_MAX) {}
		ControlNode(InterfaceGameControlID ctrl, const UI_Key& key) : command_(ctrl) { key_ = key; }
		InterfaceGameControlID command_;
		bool operator== (InterfaceGameControlID ctrl) const { return command_ == ctrl; }
		UI_Key key_;
		bool operator== (int fullkey) const { return key_ == fullkey; }
		bool serialize(Archive& ar, const char* name, const char* nameAlt);
	};
	typedef vector<ControlNode> ControlNodeList;

	struct HotKeyNode{
		HotKeyNode(int fullKey, const CallWrapperBase* func) :
			key_(fullKey){
			handler_ = func;
		}
		UI_Key key_;
		bool operator== (int fullkey) const { return key_ == fullkey; }
		const CallWrapperBase* handler_;
	};
	typedef vector<HotKeyNode> HotKeyList;

	typedef pair<InterfaceGameControlID, InterfaceGameControlID> CompatibleControlsNode;
	typedef vector<CompatibleControlsNode> CompatibleControls;
	CompatibleControls compatibleControls_;

	MTSection updateLock_;

	ControlNodeList controls_;
	HotKeyList hotKeyHandlers_;

	int interpret_;
	UI_Key interpret(UI_Key src) const;
	void defaultSettings();
};

#endif //__CONTROLS_H__
