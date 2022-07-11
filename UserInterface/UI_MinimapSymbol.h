#ifndef __UI_MINIMAP_SYMBOL_H__
#define __UI_MINIMAP_SYMBOL_H__

#include "UI_Sprite.h"

/// атрибуты символа для обозначения какого-либо объекта на миникарте
class UI_MinimapSymbol
{
public:
	UI_MinimapSymbol();

	void serialize(Archive& ar);

	enum SymbolType {
		/// прямоугольник
		SYMBOL_RECTANGLE,
		/// спрайт
		SYMBOL_SPRITE
	};

	bool operator == (const UI_MinimapSymbol& obj) const { return type == obj.type && scale == obj.scale
		&& legionColor == obj.legionColor && (!legionColor || color == obj.color)
		&& (type == SYMBOL_RECTANGLE || sprite == obj.sprite);
	}

	/// получить время жизни символа на миникарте
	int lifeTime() const;
	
public:

	/// тип изображения
	SymbolType type;
	/// собственный масштаб
	float scale;
	/// маштабировать по объекту/событию
	bool scaleByEvent;
	/// использовать цвет легиона при отрисовке
	bool legionColor;
	/// собственный цвет символа
	Color4f color;
	/// спрайт для отрисовки на миникарте
	UI_Sprite sprite;
};


/// атрибуты события на миникарте к которому можно перейти
class UI_MinimapEventStatic : public UI_MinimapSymbol
{
public:
	UI_MinimapEventStatic();
	UI_MinimapEventStatic(const UI_MinimapSymbol& symb);

	void serialize(Archive& ar);

	bool operator == (const UI_MinimapEventStatic& obj) const { return UI_MinimapSymbol::operator ==(obj); }

	float validTime() const { return validTime_; }

private:

	/// Важное событие, требует запоминания для перехода
	bool isImportant_;
	/// время на которое нужно запомнить событие
	float validTime_;
};

#endif //__UI_MINIMAP_SYMBOL_H__