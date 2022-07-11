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

/// режимы форматирования текста
enum UI_TextAlign
{
	UI_TEXT_ALIGN_LEFT		= 0,
	UI_TEXT_ALIGN_CENTER	= 1,
	UI_TEXT_ALIGN_RIGHT		= 2,
	UI_TEXT_ALIGN			= 3
};

/// режимы форматирования текста
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

/// идентификаторы событий ввода с клавиатуры/мыши
enum UI_InputEventID // HINT! новые добавлять нельзя, не влезет в ActionMode
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
	UI_ACTION_LAST_MOUSE_EVENT = UI_INPUT_MOUSE_WHEEL_DOWN // 12 бит в UI_ControlActionEvent и ActionMode
};

enum UI_ControlActionEvent
{
	UI_ACTION_MOUSE_LB = 1 << UI_INPUT_MOUSE_LBUTTON_DOWN,
	UI_ACTION_MOUSE_RB = 1 << UI_INPUT_MOUSE_RBUTTON_DOWN,
	UI_ACTION_MOUSE_MB = 1 << UI_INPUT_MOUSE_MBUTTON_DOWN,
	UI_ACTION_MOUSE_LDBL = 1 << UI_INPUT_MOUSE_LBUTTON_DBLCLICK,
	UI_ACTION_HOTKEY = 1 << UI_INPUT_HOTKEY,
	UI_ACTION_SELF_EXECUTE = (1 << UI_ACTION_LAST_MOUSE_EVENT) << 1, //13й
	UI_ACTION_FOCUS_OFF = UI_ACTION_SELF_EXECUTE << 1, //14й
	UI_ACTION_TYPE_UPDATE = UI_ACTION_FOCUS_OFF << 1, //15й
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

/// ориентация регулятора
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

/// тип выводимого списка юнитов
enum UI_ControlUnitListType{
	UI_UNITLIST_SELECTED,
	UI_UNITLIST_PRODUCTION,
	UI_UNITLIST_TRANSPORT,
	UI_UNITLIST_PRODUCTION_SQUAD
};

/// действия, которые вешаются на кнопки
enum UI_ControlActionID
{
	/// в приоритете команда скрыть контрол
	UI_ACTION_INVERT_SHOW_PRIORITY,
	/// пометка состояния
	UI_ACTION_STATE_MARK,
	// ничего не делать, разделитеть служебных действий
	UI_ACTION_NONE,
	/// расрыть шаблонные конструкции в локлизованном тексте
	UI_ACTION_EXPAND_TEMPLATE,
	/// внутренняя команда контролу
	UI_ACTION_CONTROL_COMMAND,
	/// управление другим контролом
	UI_ACTION_EXTERNAL_CONTROL,
	/// управление анимацией кнопки
	UI_ACTION_ANIMATION_CONTROL,
	/// автосмена состояний кнопки
	UI_ACTION_AUTOCHANGE_STATE,
	/// отображение/установка глобальной переменной
	UI_ACTION_GLOBAL_VARIABLE,
	/// вывод списка профайлов
	UI_ACTION_PROFILES_LIST,
	/// список онлайновых аккаунтов
	UI_ACTION_ONLINE_LOGIN_LIST,
	/// ввод имени профайла
	UI_ACTION_PROFILE_INPUT,
	/// ввод ключа
	UI_ACTION_CDKEY_INPUT,
	/// вывод списка миссий из файлов
	UI_ACTION_MISSION_LIST,
	/// список миссий с множественным выбором для быстрого старта
	UI_ACTION_MISSION_QUICK_START_FILTER,
	/// список миссий с множественным выбором
	UI_ACTION_MISSION_SELECT_FILTER,
	// раса для быстрого старта
	UI_ACTION_QUICK_START_FILTER_RACE,
	// тип игры для быстрого старта
	UI_ACTION_QUICK_START_FILTER_POPULATION,
	/// привязка к наличию сохраненных игр/реплеев
	UI_ACTION_BIND_SAVE_LIST,
	/// быстрое создание и старт online игры
	UI_ACTION_INET_QUICK_START,
	/// создать сервер сетевой игры
	UI_ACTION_LAN_CREATE_GAME,
	/// присоединиться к игре
	UI_ACTION_LAN_JOIN_GAME,
	/// создание online аккаунта
	UI_ACTION_INET_CREATE_ACCOUNT,
	/// online login
	UI_ACTION_INET_LOGIN,
	/// удалить online аккаунт
	UI_ACTION_INET_DELETE_ACCOUNT,
	/// сменить online пароль
	UI_ACTION_INET_CHANGE_PASSWORD,
	/// список людей в чате
	UI_ACTION_LAN_CHAT_USER_LIST,
	/// LAN - вывод списка сетевых игр
	UI_ACTION_LAN_GAME_LIST,
	/// LAN - сбросить сервер
	UI_ACTION_LAN_DISCONNECT_SERVER,
	/// LAN - отменить текущую операцию
	UI_ACTION_LAN_ABORT_OPERATION,
	/// LAN - ввод названия игы
	UI_ACTION_LAN_GAME_NAME_INPUT,
	/// LAN - подключиться к команде
	UI_ACTION_LAN_PLAYER_JOIN_TEAM,
	/// LAN - покинуть к команду
	UI_ACTION_LAN_PLAYER_LEAVE_TEAM,
	/// игрок сущетвует
	UI_ACTION_BIND_PLAYER,
	/// LAN - имя игрока
	UI_ACTION_LAN_PLAYER_NAME,
	/// имя игрока для статистики
	UI_ACTION_LAN_PLAYER_STATISTIC_NAME,
	/// LAN - цвет игрока
	UI_ACTION_LAN_PLAYER_COLOR,
	/// LAN - эмблема игрока
	UI_ACTION_LAN_PLAYER_SIGN,
	/// LAN - раса игрока
	UI_ACTION_LAN_PLAYER_RACE,
	/// задание сложности противника
	UI_ACTION_LAN_PLAYER_DIFFICULTY,
	/// клан игрока
	UI_ACTION_LAN_PLAYER_CLAN,
	/// игрок готов играть
	UI_ACTION_LAN_PLAYER_READY,
	/// использовать предустановленные настройки
	UI_ACTION_LAN_USE_MAP_SETTINGS,
	/// использовать предустановленные настройки
	UI_ACTION_LAN_GAME_TYPE,
	/// тип игрока
	UI_ACTION_LAN_PLAYER_TYPE,
	/// привязка к сетевой паузе
	UI_ACTION_BIND_NET_PAUSE,
	/// привязка к паузе в игре
	UI_ACTION_BIND_GAME_PAUSE,
	/// список игроков на паузе
	UI_ACTION_NET_PAUSED_PLAYER_LIST,
	/// имя текущего профиля
	UI_ACTION_PROFILE_CURRENT,
	/// подтвердить выбор профиля
	UI_ACTION_PROFILE_SELECT,
	/// создать новый профиль
	UI_ACTION_PROFILE_CREATE,
	/// удалить профиль
	UI_ACTION_PROFILE_DELETE,
	/// удалить онлайновый аккаунт из списка
	UI_ACTION_DELETE_ONLINE_LOGIN_FROM_LIST,
	/// имя игры для сохранения
	UI_ACTION_SAVE_GAME_NAME_INPUT,
	/// ввод имени реплея для сохранения
	UI_ACTION_REPLAY_NAME_INPUT,
	/// сохранить реплей
	UI_ACTION_REPLAY_SAVE,
	/// сохранить игру
	UI_ACTION_GAME_SAVE,
	/// загрузить игру
	UI_ACTION_GAME_START,
	/// поставить, снять игру с паузы
	UI_ACTION_PAUSE_GAME,
	/// управление эффектами
	UI_ACTION_POST_EFFECT,
	/// перезапустить миссию
	UI_ACTION_GAME_RESTART,
	/// удалить текущий save
	UI_ACTION_DELETE_SAVE,
	/// очистить текущую миссию
	UI_ACTION_RESET_MISSION,
	/// вывод описания миссии
	UI_ACTION_MISSION_DESCRIPTION,
	/// вывод значения параметра игрока
	UI_ACTION_PLAYER_PARAMETER,
	/// вывод значения личного параметра юнита
	UI_ACTION_UNIT_PARAMETER,
	/// вывод количества юнитов игрока
	UI_ACTION_PLAYER_UNITS_COUNT,
	/// данные статистики по игрокам
	UI_ACTION_PLAYER_STATISTICS,
	/// выбор действия при клике на мире
	UI_ACTION_CLICK_MODE,
	/// отмена действия
	UI_ACTION_CANCEL,
	/// привязка к юниту
	/// кнопка видна, когда заселекчен определённый юнит
	UI_ACTION_BIND_TO_UNIT,
	/// привязка к существованию юнита на мире
	UI_ACTION_UNIT_ON_MAP,
	/// привязка к нахождению юнита в транспорте
	UI_ACTION_UNIT_IN_TRANSPORT,
	/// переключение состояния в зависимости от параметров юнита
	UI_ACTION_BIND_TO_UNIT_STATE,
	/// проверить наличие параметров у юнита
	UI_ACTION_UNIT_HAVE_PARAMS,
	/// найти бездельника
	UI_ACTION_BIND_TO_IDLE_UNITS,
	/// привязка к типу игры
	UI_ACTION_BIND_GAME_TYPE,
	/// игра загружена
	UI_ACTION_BIND_GAME_LOADED,
	/// привязка к статусу последней сетевой операции
	UI_ACTION_BIND_ERROR_TYPE,
	/// команда выбранному юниту/зданию
	UI_ACTION_UNIT_COMMAND,
	/// команда игроку
	UI_ACTION_CLICK_FOR_TRIGGER,
	/// установка здания
	UI_ACTION_BUILDING_INSTALLATION,
	/// хватает ресурсов на установку
	UI_ACTION_BUILDING_CAN_INSTALL,
	/// селект юнита/здания
	UI_ACTION_UNIT_SELECT,
	/// селект юнита/здания
	UI_ACTION_UNIT_DESELECT,
	/// установка текущего в списке селекта
	UI_ACTION_SET_SELECTED,
	/// оперирование списками выделенных юнитов
	UI_ACTION_SELECTION_OPERATE,
	/// отображение прогресса производства
	UI_ACTION_PRODUCTION_PROGRESS,
	/// отображение прогресса производства параметров
	UI_ACTION_PRODUCTION_PARAMETER_PROGRESS,
	/// включить оружие
	UI_ACTION_ACTIVATE_WEAPON,
	/// отображение перезарядки оружия
	UI_ACTION_RELOAD_PROGRESS,
	/// опция игры
	UI_ACTION_OPTION,
	/// список графических пресетов
	UI_ACTION_OPTION_PRESET_LIST,
	/// применить настройки
	UI_ACTION_OPTION_APPLY,
	/// отменить сделанные изменения настроек
	UI_ACTION_OPTION_CANCEL,
	/// привязка к необходимости подтвердить новые настройки
	UI_ACTION_BIND_NEED_COMMIT_SETTINGS,
	/// сколько осталось времени для сохранения новых настроек
	UI_ACTION_NEED_COMMIT_TIME_AMOUNT,
	/// подтвердить новые настройки
	UI_ACTION_COMMIT_GAME_SETTINGS,
	/// откатить новые настройки
	UI_ACTION_ROLLBACK_GAME_SETTINGS,
	/// ввод хоткеев
	UI_ACTION_SET_KEYS,
	/// расширенная привязка
	UI_ACTION_BINDEX,
	/// вывод подсказки о юните
	UI_ACTION_UNIT_HINT,
	/// вывод информации (подсказки) о контроле
	UI_ACTION_UI_HINT,
	/// отображение прогресса загрузки
	UI_ACTION_LOADING_PROGRESS,
	/// переключение состояния кнопки
	UI_ACTION_CHANGE_STATE,
	/// слияние выделенных сквадов
	UI_ACTION_MERGE_SQUADS,
	/// разбиение выделенного сквада
	UI_ACTION_SPLIT_SQUAD,
	/// привязка к нахождению параметра в очереди производства
	UI_BIND_PRODUCTION_QUEUE,
	/// вывод очереди сообщений
	UI_ACTION_MESSAGE_LIST,
	/// вывод списка задач
	UI_ACTION_TASK_LIST,
	/// управление источником на мыши
	UI_ACTION_SOURCE_ON_MOUSE,
	/// Вывести текст модального сообщения
	UI_ACTION_GET_MODAL_MESSAGE,
	/// управление модальным окном
	UI_ACTION_OPERATE_MODAL_MESSAGE,
	/// время с начала игры
	UI_ACTION_SHOW_TIME,
	/// таймер обратного отсчета
	UI_ACTION_SHOW_COUNT_DOWN_TIMER,
	/// закрепить угол миникарты
	UI_ACTION_MINIMAP_ROTATION,
	/// имя для интернета
	UI_ACTION_INET_NAME,
	/// пароль для интернета
	UI_ACTION_INET_PASS,
	/// повтор пароля для интернета
	UI_ACTION_INET_PASS2,
	/// фильтр для поиска online игры по количеству игроков
	UI_ACTION_INET_FILTER_PLAYERS_COUNT,
	/// фильтр для поиска online игры по типу
	UI_ACTION_INET_FILTER_GAME_TYPE,
	/// фильтр для статистики по расе
	UI_ACTION_STATISTIC_FILTER_RACE,
	/// фильтр для статистики по типу игры
	UI_ACTION_STATISTIC_FILTER_POPULATION,
	/// запросить online статистику
	UI_ACTION_INET_STATISTIC_QUERY,
	/// вывести online статистику
	UI_ACTION_INET_STATISTIC_SHOW,
	/// строка для ввода сообщения
	UI_ACTION_CHAT_EDIT_STRING,
	/// отсылка сообщения
	UI_ACTION_CHAT_SEND_MESSAGE,
	/// отсылка сообщения в приватный/клановый чат
	UI_ACTION_CHAT_SEND_CLAN_MESSAGE,
	/// окно чата
	UI_ACTION_CHAT_MESSAGE_BOARD,
	/// игровой чат
	UI_ACTION_GAME_CHAT_BOARD,
	/// прицел в прямом управлении
	UI_ACTION_DIRECT_CONTROL_CURSOR,
	/// зарядка оружия в прямом управлении
	UI_ACTION_DIRECT_CONTROL_WEAPON_LOAD,
	/// сесть в транспорт/выйти из транспорта в прямом управлении
	UI_ACTION_DIRECT_CONTROL_TRANSPORT,
	/// подтвердить или отклонить перезапись сэйва/профиля/реплея
	UI_ACTION_CONFIRM_DISK_OP
};

/// символы для обозначения объектов на миникарте
enum UI_MinimapSymbolType{
	UI_MINIMAP_SYMBOLTYPE_NONE = 0,
	UI_MINIMAP_SYMBOLTYPE_DEFAULT,
	UI_MINIMAP_SYMBOLTYPE_SELF
};
/// символы для обозначения объектов на миникарте
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

/// действия при клике мышью
enum UI_ClickModeID
{
	UI_CLICK_MODE_NONE,
    UI_CLICK_MODE_MOVE,
	UI_CLICK_MODE_ATTACK,
	UI_CLICK_MODE_PATROL,
	UI_CLICK_MODE_REPAIR,
	UI_CLICK_MODE_RESOURCE
};

/// идентификаторы пометок, выставляемых при клике
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

/// Типы пометок.
enum UI_MarkType
{
	/// точка сбора произведённых юнитов
	UI_MARK_SHIPMENT_POINT = 0,
	/// цель атаки
	UI_MARK_ATTACK_TARGET,
	/// точка перемещения
	UI_MARK_MOVE_POINT,
	/// узел патрулирования
	UI_MARK_PATROL_POINT,
	/// цель ремонта
	UI_MARK_REPAIR_TARGET,
	/// цель атаки - unit
	UI_MARK_ATTACK_TARGET_UNIT,

	UI_MARK_TYPE_DEFAULT
};

/// Идентификаторы текстовых сообщений
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

/// Выравнивание миникарты
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

/// Состояния задач
enum UI_TaskStateID
{
	/// Назначена
	UI_TASK_ASSIGNED = 0,
	/// Выполнена
	UI_TASK_COMPLETED,
	/// Провалена
	UI_TASK_FAILED,
	/// Удалить
	UI_TASK_DELETED,

	UI_TASK_STATE_COUNT = UI_TASK_FAILED + 1
};

/// коды возврата проверок возможности действий юнита
enum Accessibility{
	// в принципе не возможно
	DISABLED = 0,
	// возможно, но прямо сейчас начать нельзя
	ACCESSIBLE,
	// все доступно, можно делать
	CAN_START
};

/// Идентификаторы этапов загрузки
enum UI_LoadProgressSectionID
{
	UI_LOADING_INITIAL = 0,	///< начальная загрузка
	UI_LOADING_UNIVERSE		///< загрузка Universe
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
	// Не курсор - просто enum, который в конце
	UI_CURSOR_LAST_ENUM
};

enum UI_OptionType
{
	UI_OPTION_UPDATE,
	UI_OPTION_APPLY,
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
	// события обрыва
	UI_NET_TERMINATE_SESSION, // восстановимый обрыв (не требует перелогина)
	UI_NET_SERVER_DISCONNECT, // полная потеря связи, требуется логин
	// статусные события последней опереции
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
	GAME_INFO_TAB_LIST,
	GAME_INFO_GAME_NAME,
	GAME_INFO_HOST_NAME,
	GAME_INFO_WORLD_NAME,
	GAME_INFO_PLAYERS_NUMBER,
	GAME_INFO_PING,
	GAME_INFO_GAME_TYPE
};

// С какой стороны выезжать кнопке при активации
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
