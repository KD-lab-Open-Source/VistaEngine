#include "StdAfx.h"
#include "Controls.h"
#include "CommonLocText.h"
#include "GameOptions.h"
#include "Serialization\Serialization.h"
#include "Serialization\EnumDescriptor.h"

WRAP_LIBRARY(ControlManager, "ControlManager", "ControlManager", "Scripts\\Content\\Controls", 0, 0);

ControlManager::ControlManager()
{
	interpret_ = GameOptions::instance().getTranslate();
	defaultSettings();
	xassert(controls_.size() >= (int)(CTRL_MAX));
}

void ControlManager::serialize(Archive &ar)
{
	for(InterfaceGameControlID i = InterfaceGameControlID(0); i < CTRL_MAX; i = static_cast<InterfaceGameControlID>(i + 1))
		ar.serialize(controls_[i].key_, getEnumName(controls_[i].command_), getEnumNameAlt(controls_[i].command_));
}

bool ControlManager::ControlNode::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	bool nodeExists = ar.serialize(command_, "command", "Команда");
	ar.serialize(key_, "key", "&Клавиша");
	return nodeExists;
}

InterfaceGameControlID ControlManager::control(int fullKey) const
{
	ControlNodeList::const_iterator it = find(controls_.begin(), controls_.end(), fullKey);
	if(it != controls_.end())
		return it->command_;

	return CTRL_MAX;
}

const UI_Key& ControlManager::key(InterfaceGameControlID ctrl) const
{
	ControlNodeList::const_iterator it = find(controls_.begin(), controls_.end(), ctrl);
	if(it != controls_.end())
		return it->key_;

	static UI_Key empty;
	return empty;
}

bool ControlManager::isCompatible(InterfaceGameControlID ctrl1, InterfaceGameControlID ctrl2) const
{
	int sz = compatibleControls_.size();
	for(int i = 0; i < sz; i++){
		if((compatibleControls_[i].first == ctrl1 && compatibleControls_[i].second == ctrl2) ||
			(compatibleControls_[i].first == ctrl2 && compatibleControls_[i].second == ctrl1))
				return true;
	}

	return false;
}

void ControlManager::unRegisterHotKey(const CallWrapperBase* func)
{
	MTAuto autolock(updateLock_);

	HotKeyList::iterator it;
	FOR_EACH(hotKeyHandlers_, it)
		if(it->handler_ == func){
			hotKeyHandlers_.erase(it);
			break;
		}
}

void ControlManager::registerHotKey(int fullKey, const CallWrapperBase* func)
{
	MTAuto autolock(updateLock_);

	unRegisterHotKey(func);
	hotKeyHandlers_.push_back(HotKeyNode(fullKey, func));
}

bool ControlManager::checkHotKey(int fullKey)
{
	MTAuto autolock(updateLock_);

	return find(hotKeyHandlers_.begin(), hotKeyHandlers_.end(), fullKey) == hotKeyHandlers_.end();
}

void ControlManager::handleKeypress(int fullKey)
{
	MTAuto autolock(updateLock_);

	HotKeyList::const_iterator it;
	FOR_EACH(hotKeyHandlers_, it)
		if(it->key_ == fullKey)
			if(it->handler_->call())
				break;
}

void ControlManager::setKey(InterfaceGameControlID ctrl, const UI_Key& key)
{
	ControlNodeList::iterator it = find(controls_.begin(), controls_.end(), ctrl);
	if(it == controls_.end())
		controls_.push_back(ControlNode(ctrl, key));
	else
		it->key_ = key;
}

UI_Key ControlManager::defaultKey(InterfaceGameControlID ctrl) const
{
	switch(ctrl){
	case CTRL_CAMERA_MOVE_UP:
		return interpret(UI_Key(VK_UP));
	case CTRL_CAMERA_MOVE_DOWN:
		return interpret(UI_Key(VK_DOWN));
	case CTRL_CAMERA_MOVE_LEFT:
		return interpret(UI_Key(VK_LEFT));
	case CTRL_CAMERA_MOVE_RIGHT:
		return interpret(UI_Key(VK_RIGHT));
	case CTRL_CAMERA_ROTATE_UP:
		return interpret(UI_Key(VK_NUMPAD8));
	case CTRL_CAMERA_ROTATE_DOWN:
		return interpret(UI_Key(VK_NUMPAD2));
	case CTRL_CAMERA_ROTATE_LEFT:
		return interpret(UI_Key(VK_NUMPAD4));
	case CTRL_CAMERA_ROTATE_RIGHT:
		return interpret(UI_Key(VK_NUMPAD6));
	case CTRL_CAMERA_ZOOM_INC:
		return interpret(UI_Key(VK_ADD));
	case CTRL_CAMERA_ZOOM_DEC:
		return interpret(UI_Key(VK_SUBTRACT));
	case CTRL_CAMERA_MOUSE_LOOK:
		return interpret(UI_Key(VK_MBUTTON));
	case CTRL_DIRECT_UP:
		return interpret(UI_Key('W'));
	case CTRL_DIRECT_DOWN:
		return interpret(UI_Key('S'));
	case CTRL_DIRECT_LEFT:
		return interpret(UI_Key('A'));
	case CTRL_DIRECT_RIGHT:
		return interpret(UI_Key('D'));
	case CTRL_DIRECT_STRAFE_LEFT:
		return interpret(UI_Key('Q'));
	case CTRL_DIRECT_STRAFE_RIGHT:
		return interpret(UI_Key('E'));
	case CTRL_PAUSE:
		return interpret(UI_Key(VK_PAUSE));
	case CTRL_MAKESHOT:
		return interpret(UI_Key(VK_F11));
	case CTRL_SHOW_UNIT_PARAMETERS:
		return interpret(UI_Key(VK_MENU));
	case CTRL_SHOW_ITEMS_HINT:
		return interpret(UI_Key('X'));
	case CTRL_TOGGLE_MUSIC:
		return interpret(UI_Key(KBD_SHIFT|'M'));
	case CTRL_TOGGLE_SOUND:
		return interpret(UI_Key(KBD_SHIFT|'S'));
	case CTRL_TIME_NORMAL:
		return interpret(UI_Key(KBD_SHIFT|KBD_CTRL|'A'));
	case CTRL_TIME_DEC:
		return interpret(UI_Key(KBD_SHIFT|'Z'));
	case CTRL_TIME_INC:
		return interpret(UI_Key(KBD_SHIFT|'A'));
	case CTRL_CAMERA_TO_EVENT:
		return interpret(UI_Key(VK_SPACE));
	case CTRL_CLICK_ACTION:
		return interpret(UI_Key(VK_LBUTTON));
	case CTRL_ATTACK:
		return interpret(UI_Key(VK_RBUTTON));
	case CTRL_ATTACK_ALTERNATE:
		return interpret(UI_Key(VK_RBUTTON));
	case CTRL_SELECT_ALL:
		return interpret(UI_Key(VK_LDBL));
	case CTRL_SELECT:
		return interpret(UI_Key(VK_LBUTTON));
	case CTRL_MOVE:
		return interpret(UI_Key(VK_RBUTTON));

	}

	return UI_Key();
}

UI_Key ControlManager::interpret(UI_Key out) const
{
	switch(interpret_){
	case UI_COMMON_TEXT_LANG_GERMAN:
		switch(out.key){
		case 'Z': out.key = 'Y'; break;
		case 'Y': out.key = 'Z'; break;
		}
		break;
	case UI_COMMON_TEXT_LANG_FRENCH:
		switch(out.key){
		case 'W': out.key = 'Z'; break;
		case 'Z': out.key = 'W'; break;
		case 'Q': out.key = 'A'; break;
		case 'A': out.key = 'Q'; break;
			}
		break;
	//case UI_COMMON_TEXT_LANG_SPANISH:
	//case UI_COMMON_TEXT_LANG_ITALIAN:
	}
	return out;
}

void ControlManager::defaultSettings()
{
	for(int i = 0; i < CTRL_MAX; ++i)
		setKey(InterfaceGameControlID(i), defaultKey(InterfaceGameControlID(i)));

	compatibleControls_.clear();
	compatibleControls_.reserve(16);

	compatibleControls_.push_back(CompatibleControlsNode(CTRL_CAMERA_MOVE_UP, CTRL_DIRECT_UP));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_CAMERA_MOVE_DOWN, CTRL_DIRECT_DOWN));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_CAMERA_MOVE_LEFT, CTRL_DIRECT_LEFT));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_CAMERA_MOVE_RIGHT, CTRL_DIRECT_RIGHT));

	compatibleControls_.push_back(CompatibleControlsNode(CTRL_SELECT, CTRL_CLICK_ACTION));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_SELECT, CTRL_ATTACK));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_CLICK_ACTION, CTRL_ATTACK));
	compatibleControls_.push_back(CompatibleControlsNode(CTRL_MOVE, CTRL_ATTACK));
}

BEGIN_ENUM_DESCRIPTOR(InterfaceGameControlID, "InterfaceGameControlID")
REGISTER_ENUM(CTRL_TIME_NORMAL, "Нормальная скорость игры")
REGISTER_ENUM(CTRL_TIME_DEC, "Замедлить игру")
REGISTER_ENUM(CTRL_TIME_INC, "Ускорить игру")
REGISTER_ENUM(CTRL_CAMERA_MOVE_UP, "Сдвинуть камеру вверх")
REGISTER_ENUM(CTRL_CAMERA_MOVE_DOWN, "Сдвинуть камеру вниз")
REGISTER_ENUM(CTRL_CAMERA_MOVE_LEFT, "Сдвинуть камеру влево")
REGISTER_ENUM(CTRL_CAMERA_MOVE_RIGHT, "Сдвинуть камеру вправо")
REGISTER_ENUM(CTRL_CAMERA_ROTATE_UP, "Прокрутить камеру вверх")
REGISTER_ENUM(CTRL_CAMERA_ROTATE_DOWN, "Прокрутить камеру вниз")
REGISTER_ENUM(CTRL_CAMERA_ROTATE_LEFT, "Прокрутить камеру влево")
REGISTER_ENUM(CTRL_CAMERA_ROTATE_RIGHT, "Прокрутить камеру вправо")
REGISTER_ENUM(CTRL_CAMERA_ZOOM_INC, "Приблизить камеру")
REGISTER_ENUM(CTRL_CAMERA_ZOOM_DEC, "Отодвинуть камеру")
REGISTER_ENUM(CTRL_CAMERA_MOUSE_LOOK, "Крутить камеру мышью")
REGISTER_ENUM(CTRL_CAMERA_MAP_SHIFT, "Двигать камеру мышью")
REGISTER_ENUM(CTRL_DIRECT_UP, "Прямое управление - вперед")
REGISTER_ENUM(CTRL_DIRECT_DOWN, "Прямое управление - назад")
REGISTER_ENUM(CTRL_DIRECT_LEFT, "Прямое управление - влево")
REGISTER_ENUM(CTRL_DIRECT_RIGHT, "Прямое управление - вправо")
REGISTER_ENUM(CTRL_DIRECT_STRAFE_LEFT, "Прямое управление - стрейф влево")
REGISTER_ENUM(CTRL_DIRECT_STRAFE_RIGHT, "Прямое управление - стрейф вправо")
REGISTER_ENUM(CTRL_PAUSE, "Пауза")
REGISTER_ENUM(CTRL_MAKESHOT, "Сохранить экран")
REGISTER_ENUM(CTRL_SHOW_UNIT_PARAMETERS, "Показать параметры юнитов")
REGISTER_ENUM(CTRL_SHOW_ITEMS_HINT, "Показать лежащие предметы")
REGISTER_ENUM(CTRL_TOGGLE_MUSIC, "Вкл/Выкл музыку")
REGISTER_ENUM(CTRL_TOGGLE_SOUND, "Вкл/Выкл звуковые эффекты")
REGISTER_ENUM(CTRL_LOAD, "Загрузить карту")
REGISTER_ENUM(CTRL_SAVE, "Сохранить карту")
REGISTER_ENUM(CTRL_CLICK_ACTION, "Клик по миру")
REGISTER_ENUM(CTRL_ATTACK, "Атаковать")
REGISTER_ENUM(CTRL_ATTACK_ALTERNATE, "Атаковать второстепенным оружием")
REGISTER_ENUM(CTRL_SELECT, "Выделить")
REGISTER_ENUM(CTRL_SELECT_ALL, "Выделить всех")
REGISTER_ENUM(CTRL_MOVE, "Идти")
REGISTER_ENUM(CTRL_MAX, "Неопределена")
REGISTER_ENUM(CTRL_CAMERA_SAVE1, "Сохранить положение камеры 1")
REGISTER_ENUM(CTRL_CAMERA_SAVE2, "Сохранить положение камеры 2")
REGISTER_ENUM(CTRL_CAMERA_SAVE3, "Сохранить положение камеры 3")
REGISTER_ENUM(CTRL_CAMERA_SAVE4, "Сохранить положение камеры 4")
REGISTER_ENUM(CTRL_CAMERA_SAVE5, "Сохранить положение камеры 5")
REGISTER_ENUM(CTRL_CAMERA_RESTORE1, "Восстановить положение камеры 1")
REGISTER_ENUM(CTRL_CAMERA_RESTORE2, "Восстановить положение камеры 2")
REGISTER_ENUM(CTRL_CAMERA_RESTORE3, "Восстановить положение камеры 3")
REGISTER_ENUM(CTRL_CAMERA_RESTORE4, "Восстановить положение камеры 4")
REGISTER_ENUM(CTRL_CAMERA_RESTORE5, "Восстановить положение камеры 5")
REGISTER_ENUM(CTRL_CAMERA_TO_EVENT, "Переместить камеру на событие")
END_ENUM_DESCRIPTOR(InterfaceGameControlID)

