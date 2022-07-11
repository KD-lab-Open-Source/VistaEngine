#ifndef __UI_GLOBAL_ATTRUBUTES_H__
#define __UI_GLOBAL_ATTRUBUTES_H__

#include "XTL\Handle.h"
#include "Serialization\LibraryWrapper.h"
#include "Serialization\EnumTable.h"

#include "UI_Enums.h"
#include "UI_Types.h"

class UI_GlobalAttributes : public LibraryWrapper<UI_GlobalAttributes>
{
public:
	UI_GlobalAttributes();

	void serialize(Archive& ar);

	const UI_Cursor* cursor(UI_CursorType cur_type) const { return cursors_(cur_type); }
	const UI_Cursor* getMoveCursor(int dir);

	float chatDelay() const { return chatDelay_; }

	const UI_MessageSetup& messageSetup(UI_MessageID index) const { return messageSetups_(index); }
	const Color4f& privateMessage() const{ return privateMessage_; }
	const Color4f& systemMessage() const{ return systemMessage_; }

	const HintAttributes& hintAttributes() const { return hintAttributes_; }
private:
	/// время отображения чат-строки в игре
	float chatDelay_;
	/// Курсоры
	typedef EnumTable<UI_CursorType, UI_CursorReference> CursorVector;
	CursorVector cursors_;
	/// цвет личного сообщения в чате
	Color4f privateMessage_;
	/// Цвет системного сообщения в чате
	Color4f systemMessage_;
	
	/// параметры подсказок для предметов на земле
	HintAttributes hintAttributes_;

	typedef EnumTable<UI_MessageID, UI_MessageSetup> MessageSetups;
	/// предопределённые сообщения, выборка по UI_MessageID
	MessageSetups messageSetups_;
};


#endif //__UI_GLOBAL_ATTRUBUTES_H__