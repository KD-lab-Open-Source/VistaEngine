#ifndef __UI_ENUMS_H__
#define __UI_ENUMS_H__

enum TripleBool {
	UI_ANY = -1,
	UI_NO = 0,
	UI_YES = 1
};

enum UI_ControlShowModeID
{
	UI_SHOW_NORMAL = 0,
	UI_SHOW_HIGHLITED,
	UI_SHOW_ACTIVATED,
	UI_SHOW_DISABLED
};

/// ������ �������������� ������
enum UI_TextAlign
{
	UI_TEXT_ALIGN_LEFT		= 0,
	UI_TEXT_ALIGN_CENTER	= 1,
	UI_TEXT_ALIGN_RIGHT		= 2,
	UI_TEXT_ALIGN			= 3
};

/// ������ �������������� ������
enum UI_TextVAlign
{
	UI_TEXT_VALIGN_TOP		= 0 << 2,
	UI_TEXT_VALIGN_CENTER	= 1 << 2,
	UI_TEXT_VALIGN_BOTTOM	= 2 << 2,
	UI_TEXT_VALIGN			= 3 << 2
};

enum UI_BlendMode
{
	UI_BLEND_NORMAL,
	UI_BLEND_ADD,
	UI_BLEND_APPEND,
	UI_BLEND_FORCE
};

/// �������������� ������� ����� � ����������/����
enum UI_InputEventID // HINT! ����� ��������� ������, �� ������ � ActionMode
{
	UI_INPUT_MOUSE_LBUTTON_DOWN = 0,
	UI_INPUT_MOUSE_RBUTTON_DOWN,
	UI_INPUT_MOUSE_MBUTTON_DOWN,
	UI_INPUT_MOUSE_LBUTTON_DBLCLICK,
	UI_INPUT_MOUSE_RBUTTON_DBLCLICK,
	UI_INPUT_MOUSE_MBUTTON_DBLCLICK,
	UI_INPUT_HOTKEY,
	UI_INPUT_MOUSE_MOVE,
	UI_INPUT_MOUSE_LBUTTON_UP,
	UI_INPUT_MOUSE_RBUTTON_UP,
	UI_INPUT_MOUSE_MBUTTON_UP,
	UI_INPUT_MOUSE_WHEEL_UP,
	UI_INPUT_MOUSE_WHEEL_DOWN, //=12
	UI_INPUT_KEY_DOWN,
	UI_INPUT_KEY_UP,
	UI_INPUT_CHAR,
	UI_ACTION_LAST_MOUSE_CLICK_EVENT = UI_INPUT_HOTKEY,
	UI_ACTION_LAST_MOUSE_EVENT = UI_INPUT_MOUSE_WHEEL_DOWN // 12 ��� � UI_ControlActionEvent � ActionMode
};

enum UI_ControlActionEvent
{
	UI_ACTION_MOUSE_LB = 1 << UI_INPUT_MOUSE_LBUTTON_DOWN,
	UI_ACTION_MOUSE_RB = 1 << UI_INPUT_MOUSE_RBUTTON_DOWN,
	UI_ACTION_MOUSE_MB = 1 << UI_INPUT_MOUSE_MBUTTON_DOWN,
	UI_ACTION_MOUSE_LDBL = 1 << UI_INPUT_MOUSE_LBUTTON_DBLCLICK,
	UI_ACTION_HOTKEY = 1 << UI_INPUT_HOTKEY,
	UI_ACTION_SELF_EXECUTE = (1 << UI_ACTION_LAST_MOUSE_EVENT) << 1, //13�
	UI_ACTION_FOCUS_OFF = UI_ACTION_SELF_EXECUTE << 1, //14�
	UI_ACTION_TYPE_UPDATE = UI_ACTION_FOCUS_OFF << 1, //15�
};

enum UI_UserSelectEvent
{
	UI_MOUSE_LBUTTON_DOWN = UI_ACTION_MOUSE_LB,
	UI_MOUSE_RBUTTON_DOWN = UI_ACTION_MOUSE_RB,
	UI_MOUSE_MBUTTON_DOWN = UI_ACTION_MOUSE_MB,
	UI_MOUSE_LBUTTON_DBLCLICK = UI_ACTION_MOUSE_LDBL,
	UI_USER_ACTION_HOTKEY = UI_ACTION_HOTKEY,
	UI_USER_ACTION_MASK = UI_MOUSE_LBUTTON_DOWN | UI_MOUSE_RBUTTON_DOWN | UI_MOUSE_MBUTTON_DOWN | UI_MOUSE_LBUTTON_DBLCLICK | UI_USER_ACTION_HOTKEY,
};

enum UI_UserEventMouseModifers
{
	UI_MOUSE_MODIFER_SHIFT	= 1 << 0,
	UI_MOUSE_MODIFER_ALT	= 1 << 1,
	UI_MOUSE_MODIFER_CTRL	= 1 << 2
};

/// ���������� ����������
enum UI_SliderOrientation
{
	UI_SLIDER_HORIZONTAL,
	UI_SLIDER_VERTICAL
};

enum UI_ControlCustomType
{
	UI_CUSTOM_CONTROL_MINIMAP = 1 << 0,
	UI_CUSTOM_CONTROL_HEAD = 1 << 1,
	UI_CUSTOM_CONTROL_SELECTION = 1 << 2
};

/// ��� ���������� ������ ������
enum UI_ControlUnitListType{
	UI_UNITLIST_SELECTED,
	UI_UNITLIST_PRODUCTION,
	UI_UNITLIST_TRANSPORT,
	UI_UNITLIST_PRODUCTION_SQUAD
};

/// ��������, ������� �������� �� ������
enum UI_ControlActionID
{
	/// � ���������� ������� ������ �������
	UI_ACTION_INVERT_SHOW_PRIORITY,
	/// ������� ���������
	UI_ACTION_STATE_MARK,
	// ������ �� ������, ����������� ��������� ��������
	UI_ACTION_NONE,
	/// ������� ��������� ����������� � ������������� ������
	UI_ACTION_EXPAND_TEMPLATE,
	/// ���������� ������� ��������
	UI_ACTION_CONTROL_COMMAND,
	/// ���������� ������ ���������
	UI_ACTION_EXTERNAL_CONTROL,
	/// ���������� ��������� ������
	UI_ACTION_ANIMATION_CONTROL,
	/// ��������� ��������� ������
	UI_ACTION_AUTOCHANGE_STATE,
	/// �����������/��������� ���������� ����������
	UI_ACTION_GLOBAL_VARIABLE,
	/// ����� ������ ���������
	UI_ACTION_PROFILES_LIST,
	/// ������ ���������� ���������
	UI_ACTION_ONLINE_LOGIN_LIST,
	/// ���� ����� ��������
	UI_ACTION_PROFILE_INPUT,
	/// ���� �����
	UI_ACTION_CDKEY_INPUT,
	/// ����� ������ ������ �� ������
	UI_ACTION_MISSION_LIST,
	/// ������ ������ � ������������� ������� ��� �������� ������
	UI_ACTION_MISSION_QUICK_START_FILTER,
	/// ������ ������ � ������������� �������
	UI_ACTION_MISSION_SELECT_FILTER,
	// ���� ��� �������� ������
	UI_ACTION_QUICK_START_FILTER_RACE,
	// ��� ���� ��� �������� ������
	UI_ACTION_QUICK_START_FILTER_POPULATION,
	/// �������� � ������� ����������� ���/�������
	UI_ACTION_BIND_SAVE_LIST,
	/// ������� �������� � ����� online ����
	UI_ACTION_INET_QUICK_START,
	/// �������� ������ online ���
	UI_ACTION_INET_REFRESH_GAME_LIST,
	/// ������� ������ ������� ����
	UI_ACTION_LAN_CREATE_GAME,
	/// �������������� � ����
	UI_ACTION_LAN_JOIN_GAME,
	/// ��� ����������� � ���������
	UI_ACTION_INET_NAT_TYPE,
	/// �������� online ��������
	UI_ACTION_INET_CREATE_ACCOUNT,
	/// online login
	UI_ACTION_INET_LOGIN,
	/// ������� online �������
	UI_ACTION_INET_DELETE_ACCOUNT,
	/// ������� online ������
	UI_ACTION_INET_CHANGE_PASSWORD,
	/// ������ ������� ����
	UI_ACTION_LAN_CHAT_CHANNEL_LIST,
	/// ����� �� ��������� ����� ����
	UI_ACTION_LAN_CHAT_CHANNEL_ENTER,
	/// �������� ���
	UI_ACTION_LAN_CHAT_CLEAR,
	/// ������ ����� � ����
	UI_ACTION_LAN_CHAT_USER_LIST,
	/// LAN - ����� ������ ������� ���
	UI_ACTION_LAN_GAME_LIST,
	/// LAN - �������� ������
	UI_ACTION_LAN_DISCONNECT_SERVER,
	/// LAN - �������� ������� ��������
	UI_ACTION_LAN_ABORT_OPERATION,
	/// LAN - ���� �������� ���
	UI_ACTION_LAN_GAME_NAME_INPUT,
	/// LAN - ������������ � �������
	UI_ACTION_LAN_PLAYER_JOIN_TEAM,
	/// LAN - �������� � �������
	UI_ACTION_LAN_PLAYER_LEAVE_TEAM,
	/// ����� ���������
	UI_ACTION_BIND_PLAYER,
	/// LAN - ��� ������
	UI_ACTION_LAN_PLAYER_NAME,
	/// ��� ������ ��� ����������
	UI_ACTION_LAN_PLAYER_STATISTIC_NAME,
	/// LAN - ���� ������
	UI_ACTION_LAN_PLAYER_COLOR,
	/// LAN - ������� ������
	UI_ACTION_LAN_PLAYER_SIGN,
	/// LAN - ���� ������
	UI_ACTION_LAN_PLAYER_RACE,
	/// ������� ��������� ����������
	UI_ACTION_LAN_PLAYER_DIFFICULTY,
	/// ���� ������
	UI_ACTION_LAN_PLAYER_CLAN,
	/// ����� ����� ������
	UI_ACTION_LAN_PLAYER_READY,
	/// ������������ ����������������� ���������
	UI_ACTION_LAN_USE_MAP_SETTINGS,
	/// ������������ ����������������� ���������
	UI_ACTION_LAN_GAME_TYPE,
	/// ��� ������
	UI_ACTION_LAN_PLAYER_TYPE,
	/// �������� � ������� �����
	UI_ACTION_BIND_NET_PAUSE,
	/// �������� � ����� � ����
	UI_ACTION_BIND_GAME_PAUSE,
	/// ������ ������� �� �����
	UI_ACTION_NET_PAUSED_PLAYER_LIST,
	/// ��� �������� �������
	UI_ACTION_PROFILE_CURRENT,
	/// ����������� ����� �������
	UI_ACTION_PROFILE_SELECT,
	/// ������� ����� �������
	UI_ACTION_PROFILE_CREATE,
	/// ������� �������
	UI_ACTION_PROFILE_DELETE,
	/// ������� ���������� ������� �� ������
	UI_ACTION_DELETE_ONLINE_LOGIN_FROM_LIST,
	/// ��� ���� ��� ����������
	UI_ACTION_SAVE_GAME_NAME_INPUT,
	/// ���� ����� ������ ��� ����������
	UI_ACTION_REPLAY_NAME_INPUT,
	/// ��������� ������
	UI_ACTION_REPLAY_SAVE,
	/// ��������� ����
	UI_ACTION_GAME_SAVE,
	/// ��������� ����
	UI_ACTION_GAME_START,
	/// ���������, ����� ���� � �����
	UI_ACTION_PAUSE_GAME,
	/// ���������� ���������
	UI_ACTION_POST_EFFECT,
	/// ������������� ������
	UI_ACTION_GAME_RESTART,
	/// ������� ������� save
	UI_ACTION_DELETE_SAVE,
	/// �������� ������� ������
	UI_ACTION_RESET_MISSION,
	/// ����� �������� ������
	UI_ACTION_MISSION_DESCRIPTION,
	/// ����� �������� ��������� ������
	UI_ACTION_PLAYER_PARAMETER,
	/// ����� �������� ������� ��������� �����
	UI_ACTION_UNIT_PARAMETER,
	/// ����� ���������� ������ ������
	UI_ACTION_PLAYER_UNITS_COUNT,
	/// ������ ���������� �� �������
	UI_ACTION_PLAYER_STATISTICS,
	/// ����� �������� ��� ����� �� ����
	UI_ACTION_CLICK_MODE,
	/// ������ ��������
	UI_ACTION_CANCEL,
	/// �������� � �����
	/// ������ �����, ����� ���������� ����������� ����
	UI_ACTION_BIND_TO_UNIT,
	/// �������� � ������������� ����� �� ����
	UI_ACTION_UNIT_ON_MAP,
	/// �������� � ���������� ����� � ����������
	UI_ACTION_UNIT_IN_TRANSPORT,
	/// ������������ ��������� � ����������� �� ���������� �����
	UI_ACTION_BIND_TO_UNIT_STATE,
	/// ��������� ������� ���������� � �����
	UI_ACTION_UNIT_HAVE_PARAMS,
	/// ����� �����������
	UI_ACTION_BIND_TO_IDLE_UNITS,
	/// �������� � ���� ����
	UI_ACTION_BIND_GAME_TYPE,
	/// ���� ���������
	UI_ACTION_BIND_GAME_LOADED,
	/// �������� � ������� ��������� ������� ��������
	UI_ACTION_BIND_ERROR_TYPE,
	/// ������� ���������� �����/������
	UI_ACTION_UNIT_COMMAND,
	/// ������� ������
	UI_ACTION_CLICK_FOR_TRIGGER,
	/// ��������� ������
	UI_ACTION_BUILDING_INSTALLATION,
	/// ������� �������� �� ���������
	UI_ACTION_BUILDING_CAN_INSTALL,
	/// ������ �����/������
	UI_ACTION_UNIT_SELECT,
	/// ������ �����/������
	UI_ACTION_UNIT_DESELECT,
	/// ��������� �������� � ������ �������
	UI_ACTION_SET_SELECTED,
	/// ������������ �������� ���������� ������
	UI_ACTION_SELECTION_OPERATE,
	/// ����������� ��������� ������������
	UI_ACTION_PRODUCTION_PROGRESS,
	/// ����������� ��������� ������������ ����������
	UI_ACTION_PRODUCTION_PARAMETER_PROGRESS,
	/// �������� ������
	UI_ACTION_ACTIVATE_WEAPON,
	/// ����������� ����������� ������
	UI_ACTION_RELOAD_PROGRESS,
	/// ����� ����
	UI_ACTION_OPTION,
	/// ������ ����������� ��������
	UI_ACTION_OPTION_PRESET_LIST,
	/// ��������� ���������
	UI_ACTION_OPTION_APPLY,
	/// �������� ��������� ��������� ��������
	UI_ACTION_OPTION_CANCEL,
	/// �������� � ������������� ����������� ����� ���������
	UI_ACTION_BIND_NEED_COMMIT_SETTINGS,
	/// ������� �������� ������� ��� ���������� ����� ��������
	UI_ACTION_NEED_COMMIT_TIME_AMOUNT,
	/// ����������� ����� ���������
	UI_ACTION_COMMIT_GAME_SETTINGS,
	/// �������� ����� ���������
	UI_ACTION_ROLLBACK_GAME_SETTINGS,
	/// ���� �������
	UI_ACTION_SET_KEYS,
	/// ����������� ��������
	UI_ACTION_BINDEX,
	/// ����� ��������� � �����
	UI_ACTION_UNIT_HINT,
	/// ����� ���������� (���������) � ��������
	UI_ACTION_UI_HINT,
	/// ����������� ��������� ��������
	UI_ACTION_LOADING_PROGRESS,
	/// ������������ ��������� ������
	UI_ACTION_CHANGE_STATE,
	/// ������� ���������� �������
	UI_ACTION_MERGE_SQUADS,
	/// ��������� ����������� ������
	UI_ACTION_SPLIT_SQUAD,
	/// �������� � ���������� ��������� � ������� ������������
	UI_BIND_PRODUCTION_QUEUE,
	/// ����� ������� ���������
	UI_ACTION_MESSAGE_LIST,
	/// ����� ������ �����
	UI_ACTION_TASK_LIST,
	/// ���������� ���������� �� ����
	UI_ACTION_SOURCE_ON_MOUSE,
	/// ������� ����� ���������� ���������
	UI_ACTION_GET_MODAL_MESSAGE,
	/// ���������� ��������� �����
	UI_ACTION_OPERATE_MODAL_MESSAGE,
	/// ����� � ������ ����
	UI_ACTION_SHOW_TIME,
	/// ������ ��������� �������
	UI_ACTION_SHOW_COUNT_DOWN_TIMER,
	/// ��������� ���� ���������
	UI_ACTION_MINIMAP_ROTATION,
	/// ��� ��� ���������
	UI_ACTION_INET_NAME,
	/// ������ ��� ���������
	UI_ACTION_INET_PASS,
	/// ������ ������ ��� ���������
	UI_ACTION_INET_PASS2,
	/// ������ ��� ������ online ���� �� ���������� �������
	UI_ACTION_INET_FILTER_PLAYERS_COUNT,
	/// ������ ��� ������ online ���� �� ����
	UI_ACTION_INET_FILTER_GAME_TYPE,
	/// ������ ��� ���������� �� ����
	UI_ACTION_STATISTIC_FILTER_RACE,
	/// ������ ��� ���������� �� ���� ����
	UI_ACTION_STATISTIC_FILTER_POPULATION,
	/// ��������� online ����������
	UI_ACTION_INET_STATISTIC_QUERY,
	/// ������� online ����������
	UI_ACTION_INET_STATISTIC_SHOW,
	/// ������ ��� ����� ���������
	UI_ACTION_CHAT_EDIT_STRING,
	/// ������� ���������
	UI_ACTION_CHAT_SEND_MESSAGE,
	/// ������� ��������� � ���������/�������� ���
	UI_ACTION_CHAT_SEND_CLAN_MESSAGE,
	/// ���� ����
	UI_ACTION_CHAT_MESSAGE_BOARD,
	/// ������� ���
	UI_ACTION_GAME_CHAT_BOARD,
	/// ������ � ������ ����������
	UI_ACTION_DIRECT_CONTROL_CURSOR,
	/// ������� ������ � ������ ����������
	UI_ACTION_DIRECT_CONTROL_WEAPON_LOAD,
	/// ����� � ���������/����� �� ���������� � ������ ����������
	UI_ACTION_DIRECT_CONTROL_TRANSPORT,
	/// ����������� ��� ��������� ���������� �����/�������/������
	UI_ACTION_CONFIRM_DISK_OP
};

/// ������� ��� ����������� �������� �� ���������
enum UI_MinimapSymbolType{
	UI_MINIMAP_SYMBOLTYPE_NONE = 0,
	UI_MINIMAP_SYMBOLTYPE_DEFAULT,
	UI_MINIMAP_SYMBOLTYPE_SELF
};
/// ������� ��� ����������� �������� �� ���������
enum UI_MinimapSymbolID
{
	UI_MINIMAP_SYMBOL_DEFAULT = 0,
	UI_MINIMAP_SYMBOL_UNIT,
	UI_MINIMAP_SYMBOL_UDER_ATTACK,
	UI_MINIMAP_SYMBOL_CLAN_UDER_ATTACK,
	UI_MINIMAP_SYMBOL_ADD_UNIT,
	UI_MINIMAP_SYMBOL_BUILD_FINISH,
	UI_MINIMAP_SYMBOL_UPGRAGE_FINISH,
	UI_MINIMAP_SYMBOL_ACTION_CLICK,
	UI_MINIMAP_SYMBOL_MAX
};

enum ObjectTransparentMode
{
	OBJT_VOID = 0,
	OBJT_EQUITANT = 1,
	OBJT_TRANSPARENT = 2
};

/// �������� ��� ����� �����
enum UI_ClickModeID
{
	UI_CLICK_MODE_NONE,
    UI_CLICK_MODE_MOVE,
	UI_CLICK_MODE_ATTACK,
	UI_CLICK_MODE_PATROL,
	UI_CLICK_MODE_REPAIR,
	UI_CLICK_MODE_RESOURCE
};

/// �������������� �������, ������������ ��� �����
enum UI_ClickModeMarkID
{
	UI_CLICK_MARK_MOVEMENT = 0,
	UI_CLICK_MARK_ATTACK,
	UI_CLICK_MARK_REPAIR,
	UI_CLICK_MARK_MOVEMENT_WATER,
	UI_CLICK_MARK_ATTACK_UNIT,
	UI_CLICK_MARK_PATROL,

	UI_CLICK_MARK_SIZE
};

/// ���� �������.
enum UI_MarkType
{
	/// ����� ����� ������������ ������
	UI_MARK_SHIPMENT_POINT = 0,
	/// ���� �����
	UI_MARK_ATTACK_TARGET,
	/// ����� �����������
	UI_MARK_MOVE_POINT,
	/// ���� ��������������
	UI_MARK_PATROL_POINT,
	/// ���� �������
	UI_MARK_REPAIR_TARGET,
	/// ���� ����� - unit
	UI_MARK_ATTACK_TARGET_UNIT,

	UI_MARK_TYPE_DEFAULT
};

/// �������������� ��������� ���������
enum UI_MessageID
{
	UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_BUILDING,
	UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_SHOOTING,
	UI_MESSAGE_UNIT_LIMIT_REACHED,

	UI_MESSAGE_TASK_ASSIGNED,
	UI_MESSAGE_TASK_COMPLETED,
	UI_MESSAGE_TASK_FAILED,

	UI_MESSAGE_ID_MAX
};

/// ������������ ���������
enum UI_Align
{
	UI_ALIGN_BOTTOM_RIGHT,
	UI_ALIGN_BOTTOM_LEFT,
	UI_ALIGN_TOP_RIGHT,
	UI_ALIGN_TOP_LEFT,
	UI_ALIGN_RIGHT,
	UI_ALIGN_LEFT,
	UI_ALIGN_BOTTOM,
	UI_ALIGN_TOP,
	UI_ALIGN_CENTER,
};

/// ��������� �����
enum UI_TaskStateID
{
	/// ���������
	UI_TASK_ASSIGNED = 0,
	/// ���������
	UI_TASK_COMPLETED,
	/// ���������
	UI_TASK_FAILED,
	/// �������
	UI_TASK_DELETED,

	UI_TASK_STATE_COUNT = UI_TASK_FAILED + 1
};

/// ���� �������� �������� ����������� �������� �����
enum Accessibility{
	// � �������� �� ��������
	DISABLED = 0,
	// ��������, �� ����� ������ ������ ������
	ACCESSIBLE,
	// ��� ��������, ����� ������
	CAN_START
};

/// �������������� ������ ��������
enum UI_LoadProgressSectionID
{
	UI_LOADING_INITIAL = 0,	///< ��������� ��������
	UI_LOADING_UNIVERSE		///< �������� Universe
};

enum UI_CursorType
{
	UI_CURSOR_PASSABLE = 0,
	UI_CURSOR_IMPASSABLE,
	UI_CURSOR_WATER,
	UI_CURSOR_PLAYER_OBJECT,
	UI_CURSOR_ALLY_OBJECT,
	UI_CURSOR_ENEMY_OBJECT,
	UI_CURSOR_ITEM_OBJECT,
	UI_CURSOR_ITEM_CAN_PIC,
	UI_CURSOR_ITEM_EXTRACT,
	UI_CURSOR_INTERFACE,
	UI_CURSOR_MAIN_MENU,
	UI_CURSOR_WAITING,
	UI_CURSOR_MOUSE_LBUTTON_DOWN,
	UI_CURSOR_MOUSE_RBUTTON_DOWN,
	UI_CURSOR_WALK,
	UI_CURSOR_WALK_DISABLED,
	UI_CURSOR_PATROL,
	UI_CURSOR_PATROL_DISABLED,
	UI_CURSOR_ATTACK,
	UI_CURSOR_PLAYER_CONTROL_DISABLED,
	UI_CURSOR_FRIEND_ATTACK,
	UI_CURSOR_ATTACK_DISABLED,
	UI_CURSOR_WORLD_OBJECT,
	UI_CURSOR_SCROLL_UP,
	UI_CURSOR_SCROLL_LEFT,
	UI_CURSOR_SCROLL_UPLEFT,
	UI_CURSOR_SCROLL_RIGHT,
	UI_CURSOR_SCROLL_UPRIGHT,
	UI_CURSOR_SCROLL_BOTTOM,
	UI_CURSOR_SCROLL_BOTTOMLEFT,
	UI_CURSOR_SCROLL_BOTTOMRIGHT,
	UI_CURSOR_ROTATE,
	UI_CURSOR_ROTATE_DIRECT_CONTROL,
	UI_CURSOR_DIRECT_CONTROL_ATTACK,
	UI_CURSOR_DIRECT_CONTROL,
	UI_CURSOR_TRANSPORT,
	UI_CURSOR_CAN_BUILD,
	UI_CURSOR_TELEPORT,
	// �� ������ - ������ enum, ������� � �����
	UI_CURSOR_LAST_ENUM
};

enum UI_OptionType
{
	UI_OPTION_UPDATE,
	UI_OPTION_APPLY,
	UI_OPTION_DEFAULT,
	UI_OPTION_CANCEL
};

enum PlayControlAction
{
	PLAY_ACTION_PLAY,
	PLAY_ACTION_PAUSE,
	PLAY_ACTION_STOP,
	PLAY_ACTION_RESTART
};

enum StateMarkType 
{
	UI_STATE_MARK_NONE,
	UI_STATE_MARK_UNIT_SELF_ATTACK,
	UI_STATE_MARK_UNIT_WEAPON_MODE,
	UI_STATE_MARK_UNIT_WALK_ATTACK_MODE,
	UI_STATE_MARK_UNIT_AUTO_TARGET_FILTER,
	UI_STATE_MARK_DIRECT_CONTROL_CURSOR
};

enum UI_DirectControlCursorType
{
	UI_DIRECT_CONTROL_CURSOR_NONE,
	UI_DIRECT_CONTROL_CURSOR_ENEMY,
	UI_DIRECT_CONTROL_CURSOR_ALLY,
	UI_DIRECT_CONTROL_CURSOR_TRANSPORT
};

enum UI_NetStatus
{
	// ������� ������
	UI_NET_TERMINATE_SESSION, // ������������� ����� (�� ������� ����������)
	UI_NET_SERVER_DISCONNECT, // ������ ������ �����, ��������� �����
	// ��������� ������� ��������� ��������
	UI_NET_WAITING,
	UI_NET_ERROR,
	UI_NET_OK
};


enum GameTuneOptionType {
	TUNE_PLAYER_NAME,
	TUNE_PLAYER_STAT_NAME,
	TUNE_PLAYER_RACE,
	TUNE_PLAYER_DIFFICULTY,
	TUNE_PLAYER_COLOR,
	TUNE_PLAYER_CLAN,
	TUNE_PLAYER_EMBLEM,
	TUNE_PLAYER_TYPE,
	TUNE_BUTTON_JOIN,
	TUNE_BUTTON_KICK,
	TUNE_FLAG_READY,
	TUNE_FLAG_VARIABLE
};

enum ScenarioGameType {
	SCENARIO_GAME_TYPE_PREDEFINE,
	SCENARIO_GAME_TYPE_CUSTOM,
	SCENARIO_GAME_TYPE_ANY
};

enum TeamGameType {
	TEAM_GAME_TYPE_INDIVIDUAL,
	TEAM_GAME_TYPE_TEEM,
	TEAM_GAME_TYPE_ANY
};

enum GameListInfoType {
	GAME_INFO_START_STATUS,
	GAME_INFO_TAB,
	GAME_INFO_GAME_NAME,
	GAME_INFO_HOST_NAME,
	GAME_INFO_WORLD_NAME,
	GAME_INFO_PLAYERS_NUMBER,
	GAME_INFO_PLAYERS_CURRENT,
	GAME_INFO_PLAYERS_MAX,
	GAME_INFO_PING,
	GAME_INFO_NAT_TYPE,
	GAME_INFO_NAT_COMPATIBILITY,
	GAME_INFO_GAME_TYPE
};

typedef vector<GameListInfoType> GameListInfoTypes;

// � ����� ������� �������� ������ ��� ���������
enum ActivationMove {
	ACTIVATION_MOVE_LEFT = 0,
	ACTIVATION_MOVE_BOTTOM,
	ACTIVATION_MOVE_RIGHT,
	ACTIVATION_MOVE_TOP,
	ACTIVATION_MOVE_CENTER
};

enum UI_DiskOpID
{
	UI_DISK_OP_NONE = 0,
	UI_DISK_OP_SAVE_GAME,
	UI_DISK_OP_SAVE_REPLAY,
	UI_DISK_OP_SAVE_PROFILE
};

enum GameStatisticType {
	STAT_FREE_FOR_ALL = 0,
	STAT_1_VS_1,
	STAT_2_VS_2,
	STAT_3_VS_3
};

const float UI_DIRECT_CONTROL_CURSOR_DIST	= 500.f;

#endif /* __UI_ENUMS_H__ */
