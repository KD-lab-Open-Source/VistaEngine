#include "StdAfx.h"
#include "CommonLocText.h"
#include "Serialization.h"
#include "skey.h"

WRAP_LIBRARY(CommonLocText, "CommonLocText", "CommonLocText", "Scripts\\Content\\CommonLocTexts", 0, false);

CommonLocText::CommonLocText()
{
	locTexts_.resize(UI_COMMON_TEXT_LAST_ENUM);

}

CommonLocText::~CommonLocText()
{
}

const char* CommonLocText::getText(UI_CommonLocText text_id) const
{
	xassert(text_id >= 0 && text_id < UI_COMMON_TEXT_LAST_ENUM);
	return locTexts_[text_id].c_str();
}

UI_CommonLocText CommonLocText::getId(const char* text) const
{
	if(text){
		int idx = 0;
		LocTextVector::const_iterator it;
		FOR_EACH(locTexts_, it)
			if(!strcmp(it->c_str(), text))
				return UI_CommonLocText(idx);
			else
				++idx;
	}
	dassert(0 && "ID ������ �� ������");
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
	
	ar.serialize(anyTexts_, "anyTexts", "������ ��� �����������");

	ar.openStruct("vk_key_names", "����������� ������", "sKey");
	sKey::serializeLocale(ar);
	ar.closeStruct("vk_key_names");
}

const char* getLocString(UI_CommonLocText id, const char* def)
{
	const char* text = CommonLocText::instance().getText(id);
	return text && *text ? text : def;
}

UI_CommonLocText getLocStringId(const char* text)
{
	return CommonLocText::instance().getId(text);
}

BEGIN_ENUM_DESCRIPTOR(UI_CommonLocText, "UI_CommonLocText")
REGISTER_ENUM(UI_COMMON_TEXT_YES, "��")
REGISTER_ENUM(UI_COMMON_TEXT_NO, "���")
REGISTER_ENUM(UI_COMMON_TEXT_DEFAULT, "�����")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_FREE_FOR_ALL, "������ �� ����")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_1_VS_1, "1 �� 1")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_2_VS_2, "2 �� 2")
REGISTER_ENUM(UI_COMMON_TEXT_STAT_3_VS_3, "3 �� 3")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_UNITS_SIZE, "����� ���������� ������� �� �����")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_GAME, "����� ���� (custom ��� predefine)")
REGISTER_ENUM(UI_COMMON_TEXT_ALL_GAMES, "����� ���� �� ������� (��� ����)")
REGISTER_ENUM(UI_COMMON_TEXT_ANY_RACE, "����� ����")
REGISTER_ENUM(UI_COMMON_TEXT_GAME_RUNNING, "����: ���� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_GAME_NOT_RUNNING, "����: ���� �� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_OPEN, "��� inet ����������� 1")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_MODERATE, "��� inet ����������� 2")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_TYPE_STRICT, "��� inet ����������� 3")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_COMPATIBLE, "����������� � inet ����������")
REGISTER_ENUM(UI_COMMON_TEXT_NAT_INCOMPATIBLE, "����������� � inet ������������")
REGISTER_ENUM(UI_COMMON_TEXT_CUSTOM_GAME, "Custom game")
REGISTER_ENUM(UI_COMMON_TEXT_PREDEFINE_GAME, "Predefine game")
REGISTER_ENUM(UI_COMMON_TEXT_INDIVIDUAL_LAN_GAME, "������������� ������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_CONNECTING, "�����������...")
REGISTER_ENUM(UI_COMMON_TEXT_YOU_ENTER_CHAT_ROOM, "�� ����� � ���: �����-��")
REGISTER_ENUM(UI_COMMON_TEXT_USER_ENTER_CHAT_ROOM, "���-��: ����� � ���")
REGISTER_ENUM(UI_COMMON_TEXT_USER_LEAVE_CHAT_ROOM, "���-��: ����� �� ����")
REGISTER_ENUM(UI_COMMON_TEXT_PLAYER_DISCONNECTED, "����� ����������")
REGISTER_ENUM(UI_COMMON_TEXT_TEEM_LAN_GAME, "��������� ������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CANT_CONNECT, "�� ������� ����������� � �������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DESINCH, "���� �������������������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DISCONNECT, "������ ����� �����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON, "������ ����� �����: ������������ ������������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_SESSION_TERMINATE, "����� ������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION, "������ ����������� � �������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME, "������ ����������� � ����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_RUN, "������ �����������: ���� ��� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_FULL, "������ �����������: ���� ���������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_INCORRECT_VERSION, "�� ��������� ������ ����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CHANGE_PASSWORD, "������� ������ �� �������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_DELETE, "������� ������� �� �������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD, "������ ������ ��� ������ �� ���������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_UNKNOWN_NAME, "������ ������������ ���")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_NAME, "��������/�������� ���")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_VULGAR_NAME, "������/���������� ���")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD, "�������� ������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_LOCKED, "������� ������������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_NAME_EXIST, "��� ������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE, "������ �������� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_MAX, "��������� ���������� ���������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_BAD_LIC, "������ ��������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_CREATE_GAME, "������ �������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_UNKNOWN, "����������� ������� ������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_OPEN, "������ ��������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_SAVE, "������ ����������")
REGISTER_ENUM(UI_COMMON_TEXT_ERROR_GRAPH_INIT, "������ ������������� �������")
REGISTER_ENUM(UI_COMMON_TEXT_TIME_AM, "����� �� �������")
REGISTER_ENUM(UI_COMMON_TEXT_TIME_PM, "����� ����� �������")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_OPEN, "���� ������")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_CLOSED, "���� ������")
REGISTER_ENUM(UI_COMMON_TEXT_SLOT_AI, "���� ������ AI")
REGISTER_ENUM(UI_COMMON_TEXT_AI, "���������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_ENGLISH, "����������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_RUSSIAN, "�������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_GERMAN, "��������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_FRENCH, "����������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_SPANISH, "���������")
REGISTER_ENUM(UI_COMMON_TEXT_LANG_ITALIAN, "�����������")
REGISTER_ENUM(UI_COMMON_TEXT_CUSTOM_PRESET, "���������������� ����� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_DISABLED, "Disabled")
REGISTER_ENUM(UI_COMMON_TEXT_SHADOW_CIRCLE, "������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_SHADOW_BAD, "������ ����")
REGISTER_ENUM(UI_COMMON_TEXT_SHADOW_GOOD, "������� ����")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_LOW, "�������� ������")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_MEDIUM, "�������� �������")
REGISTER_ENUM(UI_COMMON_TEXT_TEXTURE_GOOD, "�������� �������")
REGISTER_ENUM(UI_COMMON_TEXT_EFFECT_SMOOTH_TERRAIN, "�������� �������� ������ � ������")
REGISTER_ENUM(UI_COMMON_TEXT_EFFECT_SMOOTH_FULL, "�������� �������� ������")
REGISTER_ENUM(UI_COMMON_TEXT_HIGH_QA, "������� ��������")
REGISTER_ENUM(UI_COMMON_TEXT_LOW_QA, "������ ��������")
END_ENUM_DESCRIPTOR(UI_CommonLocText)