#include "stdafx.h"
#include "UI_GlobalAttributes.h"
#include "UI_Render.h"
#include "CommonLocText.h"

WRAP_LIBRARY(UI_GlobalAttributes, "UI_GlobalAttributes", "UI_GlobalAttributes", "Scripts\\Content\\UI_GlobalAttributes", 0, 0);

UI_GlobalAttributes::UI_GlobalAttributes()
{
	chatDelay_ = 5.f;
	privateMessage_ = Color4f(0, 1, 0, 1); 
	systemMessage_ = Color4f(0, 1, 0, 1);
}

const UI_Cursor* UI_GlobalAttributes::getMoveCursor(int dir){
	switch(dir){
		case 1:
			return cursor(UI_CURSOR_SCROLL_UP);
		case 2:
			return cursor(UI_CURSOR_SCROLL_LEFT);
		case 3:
			return cursor(UI_CURSOR_SCROLL_UPLEFT);
		case 4:
			return cursor(UI_CURSOR_SCROLL_RIGHT);
		case 5:
			return cursor(UI_CURSOR_SCROLL_UPRIGHT);
		case 8:
			return cursor(UI_CURSOR_SCROLL_BOTTOM);
		case 10:
			return cursor(UI_CURSOR_SCROLL_BOTTOMLEFT);
		case 12:
			return cursor(UI_CURSOR_SCROLL_BOTTOMRIGHT);
		default:
			return 0;
	}
}

void UI_GlobalAttributes::serialize(Archive& ar)
{
	ar.serialize(UI_Render::instance(), "UI_Render", "UI_Render");

	UI_Task::serializeColors(ar);

	ar.serialize(privateMessage_, "privateMessage", "Цвет личного/кланового сообщения");
	ar.serialize(systemMessage_, "systemMessage", "Цвет системного сообщения");
	ar.serialize(chatDelay_, "chatMessageDelay", "Время отображения игрового чат-сообщения (сек)");

#ifdef MAELSTROM_DATA
	// The table is the same table -- one library name per UI_CursorType, and every name
	// but UI_CURSOR_ASSEMBLY_POINT is present -- but pre-2008 it sat flat among the
	// attribute's own fields rather than inside a "cursors" block. Serializing the
	// EnumTable directly reads the entries at this level, which is where they are.
	//
	// Left in the block, the name never matches, the table keeps its constructed empty
	// references, and every cursor() answers null. UI_LogicDispatcher::setCursor takes
	// that for "no cursor" and falls back to setDefaultCursor() -- so the whole game runs
	// on Scripts/Resource/Cursors/default.cur, a single static frame, instead of the
	// animated pointer and hourglass.
	cursors_.serialize(ar);
#else
	ar.serialize(cursors_, "cursors", "Курсоры");
#endif

	ar.serialize(messageSetups_, "messageSetups", "Сообщения");

	ar.serialize(hintAttributes_, "hintAttributes", "Отображение подсказок для лежащих предметов");

	if(ar.isEdit())
		ar.serialize(CommonLocText::instance(), "locTexts", "Общие тексты для локализации");
}
