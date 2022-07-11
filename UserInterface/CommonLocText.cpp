#include "StdAfx.h"
#include "CommonLocText.h"
#include "Serialization\Serialization.h"
#include "Serialization\EnumDescriptor.h"
#include "UserInterface\UI_Key.h"

WRAP_LIBRARY(CommonLocText, "CommonLocText", "CommonLocText", "Scripts\\Content\\CommonLocTexts", 0, 0);

CommonLocText::CommonLocText()
{
	locTexts_.resize(UI_COMMON_TEXT_LAST_ENUM);

}

CommonLocText::~CommonLocText()
{
}

const wchar_t* CommonLocText::getText(UI_CommonLocText text_id) const
{
	xassert(text_id >= 0 && text_id < UI_COMMON_TEXT_LAST_ENUM);
	return locTexts_[text_id].c_str();
}

UI_CommonLocText CommonLocText::getId(const wchar_t* text) const
{
	if(text){
		int idx = 0;
		LocTextVector::const_iterator it;
		FOR_EACH(locTexts_, it)
			if(!wcscmp(it->c_str(), text))
				return UI_CommonLocText(idx);
			else
				++idx;
	}
	dassert(0 && "ID текста не найден");
	return UI_COMMON_TEXT_LAST_ENUM;
}

void CommonLocText::update()
{
	LocTextVector::iterator it;
	FOR_EACH(locTexts_, it){
		it->update();
	}
}

void CommonLocText::serialize(Archive& ar)
{
	for(int idx = 0; idx < UI_COMMON_TEXT_LAST_ENUM; ++idx)
		ar.serialize(locTexts_[idx], getEnumName(UI_CommonLocText(idx)), getEnumNameAlt(UI_CommonLocText(idx)));
	
	ar.serialize(anyTexts_, "anyTexts", "Тексты для локализации");

	if(ar.openStruct(*this, "vk_key_names", "локализация клавиш")){
		UI_Key::serializeLocale(ar);
		ar.closeStruct("vk_key_names");
	}
}

const wchar_t* getLocString(UI_CommonLocText id, const wchar_t* def)
{
	const wchar_t* text = CommonLocText::instance().getText(id);
	return text && *text ? text : def;
}

UI_CommonLocText getLocStringId(const wchar_t* text)
{
	return CommonLocText::instance().getId(text);
}

BEGIN_ENUM_DESCRIPTOR(UI_CommonLocText, "UI_CommonLocText")
REGISTER_ENUM(UI_COMMON_TEXT_YES, "Да")
REGISTER_ENUM(UI_COMMON_TEXT_NO, "Нет")
REGISTER_ENUM(UI_COMMON_TEXT_DEFAULT, "Любой")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_FREE_FOR_ALL, "Каждый за себя")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_1_VS_1, "1 на 1")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_2_VS_2, "2 на 2")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_3_VS_3, "3 на 3")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_UNITS_SIZE, "Любое количество игроков на карте")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_GAME, "Любая игра (custom или predefine)")
REGISTER_ENUM(UI_COMMON_TEXT_ALL_GAMES, "Любая игра из фильтра (все игры)")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_RACE, "Любая раса")
REGISTER_ENUM(UI_COMMON_TEXT_GAME_RUNNING, "Знак: игра запущена")
REGISTER_ENUM(UI_COMMON_TEXT_GAME_NOT_RUNNING, "Знак: игра НЕ запущена")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_OPEN, "Тип inet подключения 1")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_MODERATE, "Тип inet подключения 2")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_STRICT, "Тип inet подключения 3")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_COMPATIBLE, "Подключения к inet совместимы")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_INCOMPATIBLE, "Подключения к inet НЕсовместимы")
REGISTER_ENUM(UI_COMMON_TEXT_CUSTOM_GAME, "Custom game")
REGISTER_ENUM(UI_COMMON_TEXT_PREDEFINE_GAME, "Predefine game")
REGISTER_ENUM(UI_COMMON_TEXT_INDIVIDUAL_LAN_GAME, "Индивидульная сетевая игра")
REGISTER_ENUM(UI_COMMON_TEXT_CONNECTING, "Подключение...")
REGISTER_ENUM(UI_COMMON_TEXT_YOU_ENTER_CHAT_ROOM, "Вы вошли в чат: такой-то")
REGISTER_ENUM(UI_COMMON_TEXT_USER_ENTER_CHAT_ROOM, "кто-то: вошел в чат")
REGISTER_ENUM(UI_COMMON_TEXT_USER_LEAVE_CHAT_ROOM, "кто-то: вышел из чата")
REGISTER_ENUM(UI_COMMON_TEXT_PLAYER_DISCONNECTED, "Игрок отключился")
REGISTER_ENUM(UI_COMMON_TEXT_TEEM_LAN_GAME, "Командная сетевая игра")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CANT_CONNECT, "Не удалось подключится к серверу")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DESINCH, "Игра рассинхронизирована")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DISCONNECT, "Полный обрыв связи")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON, "Полный обрыв связи: зафиксирован множественный вход")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_SESSION_TERMINATE, "Обрыв сессии")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION, "Ошибка подключения к сервису")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME, "Ошибка подключения к игре")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_RUN, "Ошибка подключения: Игра уже запущена")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_FULL, "Ошибка подключения: Игра заполнена")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_INCORRECT_VERSION, "Не совпадает версия игры")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CHANGE_PASSWORD, "Сменить пароль не удалось")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_DELETE, "Удалить аккаунт не удалось")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD, "Пароль пустой или пароли не совпадают")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_UNKNOWN_NAME, "Такого пользователя нет")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_NAME, "Неверное/Короткое имя")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_VULGAR_NAME, "Плохое/Вульгарное имя")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD, "Неверный пароль")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_LOCKED, "Аккаунт заблокирован")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_NAME_EXIST, "Имя занято")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE, "Ошибка создания аккаунта")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_MAX, "Превышено количество аккаунтов")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_BAD_LIC, "Плохая лицензия")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CREATE_GAME, "Ошибка создания игры")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_UNKNOWN, "Неизвестная сетевая ошибка")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_OPEN, "Ошибка открытия")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_SAVE, "Ошибка сохранения")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_GRAPH_INIT, "Ошибка инициализации графики")
REGISTER_ENUM(UI_COMMON_TEXT_TIME_AM, "время до полудня")
REGISTER_ENUM(UI_COMMON_TEXT_TIME_PM, "время после полудня")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_OPEN, "слот открыт")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_CLOSED, "слот закрыт")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_AI, "слот выбран AI")
REGISTER_ENUM(UI_COMMON_TEXT_AI, "Компьютер")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_ENGLISH, "английский")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_RUSSIAN, "русский")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_GERMAN, "немецкий")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_FRENCH, "французкий")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_SPANISH, "испанский")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_ITALIAN, "итальянский")
REGISTER_ENUM(UI_COMMON_TEXT_CUSTOM_PRESET, "пользовательский набор настроек")
REGISTER_ENUM(UI_COMMON_TEXT_DISABLED, "Disabled")
REGISTER_ENUM(UI_COMMON_TEXT_SHADOW_BAD, "Плохие тени")
REGISTER_ENUM(UI_COMMON_TEXT_SHADOW_GOOD, "Хорошие тени")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_LOW, "Текстуры плохие")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_MEDIUM, "Текстуры средние")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_GOOD, "Текстуры хорошие")
REGISTER_ENUM(UI_COMMON_TEXT_EFFECT_SMOOTH_TERRAIN, "Размытие эффектов только с землей")
REGISTER_ENUM(UI_COMMON_TEXT_EFFECT_SMOOTH_FULL, "Размытие эффектов полное")
REGISTER_ENUM(UI_COMMON_TEXT_HIGH_QA, "Высокое качество")
REGISTER_ENUM(UI_COMMON_TEXT_LOW_QA, "Низкое качество")
END_ENUM_DESCRIPTOR(UI_CommonLocText)