#include "StdAfx.h"

#include "Options.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"
//#include "Serialization\EditArchive.h"
#include "Serialization\StringTableImpl.h"

#include "UserInterface\UI_Render.h"
#include "DebugPrm.h"
#include "Game\GameOptions.h"
#include "Serialization\EnumDescriptor.h"
#include "UnicodeConverter.h"

Options::Options ()
: grid_metrics_(GRID_METRICS_RELATIVE)
, grid_size_(0.01f, 0.01f / 0.75f) 
, grid_size_pixels_(16, 16) 
, grid_size_count_(120, 90) 
, show_grid_ (true)
, snap_to_grid_ (true)
, snap_to_border_ (false)
, show_border_ (false)
, ruler_width_ (0.02f)
, large_grid_size_(4, 4)
, zoom_mode_(ZOOM_SHOW_ALL)
, backgroundColor_(0, 0, 0, 255)
, workspaceColor_(96, 92, 92, 255)
, resolution_(1024, 768)
{
}

typedef Options::Guide OptionsGuide;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(OptionsGuide, Type, "Options::Guide::Type")
REGISTER_ENUM_ENCLOSED(OptionsGuide, HORIZONTAL,   "HORIZONTAL");
REGISTER_ENUM_ENCLOSED(OptionsGuide, VERTICAL,   "VERTICAL");
END_ENUM_DESCRIPTOR_ENCLOSED(OptionsGuide, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Options, ZoomMode, "Options::Guide::Type")
REGISTER_ENUM_ENCLOSED(Options, ZOOM_SHOW_ALL,   "Показывать все");
REGISTER_ENUM_ENCLOSED(Options, ZOOM_ONE_TO_ONE, "Один к одному (100%)");
REGISTER_ENUM_ENCLOSED(Options, ZOOM_CUSTOM, "Ручной")
END_ENUM_DESCRIPTOR_ENCLOSED(Options, ZoomMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Options, GridMetrics, "Options::GridMetrics")
REGISTER_ENUM_ENCLOSED(Options, GRID_METRICS_RELATIVE, "В относительных координатах (от 0.0 до 1.0)");
REGISTER_ENUM_ENCLOSED(Options, GRID_METRICS_PIXELS, "В пикселах");
REGISTER_ENUM_ENCLOSED(Options, GRID_METRICS_COUNT, "По количеству ячеек сетки");
END_ENUM_DESCRIPTOR_ENCLOSED(Options, GridMetrics)

void Options::serialize (Archive& ar)
{

	XBuffer buf;
	buf <= resolution_.x < "*" <= resolution_.y;
	ComboListString resolution(w2a(GameOptions::instance().getUnfilteredList(OPTION_SCREEN_SIZE)).c_str(), buf);
	ar.serialize(resolution, "resolution", "Разрешение экрана");
	if(ar.isInput()){
		sscanf(resolution.value().c_str(), "%i*%i", &resolution_.x, &resolution_.y);
	}

    ar.openBlock("", "Сетка");
    ar.serialize(show_grid_, "show_grid_", "Показывать сетку");
    ar.serialize(snap_to_grid_, "snap_to_grid_", "Привязывать к сетке");

    ar.serialize(grid_metrics_, "grid_metrics", "Режим задания координат сетки");
	if(ar.isEdit()){
		switch(grid_metrics_){
		case GRID_METRICS_RELATIVE:
	        ar.serialize(grid_size_.x, "grid_size_x", "Размер по горизонтали");
	        ar.serialize(grid_size_.y, "grid_size_y", "Размер по вертикали");
			break;
		case GRID_METRICS_PIXELS:
			ar.serialize(grid_size_pixels_.x, "grid_size_pixels_x", "Размер сетки по горизонтали (в пикселах)");
			ar.serialize(grid_size_pixels_.y, "grid_size_pixels_y", "Размер сетки по вертикали (в пикселах)");
			break;
		case GRID_METRICS_COUNT:
			ar.serialize(grid_size_count_.x, "grid_size_count_x", "Размер сетки по горизонтали (в ячейках)");
			ar.serialize(grid_size_count_.y, "grid_size_count_y", "Размер сетки по вертикали (в ячейках)");
			break;
		}
	    ar.serialize(large_grid_size_.x, "large_grid_size_x", "Частота крупной сетки по горизонтали");
	    ar.serialize(large_grid_size_.y, "large_grid_size_y", "Частота крупной сетки по вертикали");
	}
	else{
        ar.serialize(grid_size_, "grid_size_", 0);
        ar.serialize(grid_size_pixels_, "grid_size_pixels", 0);
        ar.serialize(grid_size_count_, "grid_size_count", 0);
	    ar.serialize(large_grid_size_, "large_grid_size_", "Частота крупной сетки");
	}

	grid_size_.x = clamp(grid_size_.x, 0.0001f, 1.0f);
	grid_size_.y = clamp(grid_size_.y, 0.0001f, 1.0f);
	grid_size_pixels_.x = clamp(grid_size_pixels_.x, 1, 10000);
	grid_size_pixels_.y = clamp(grid_size_pixels_.y, 1, 10000);
	grid_size_count_.x = clamp(grid_size_count_.x, 1, 10000);
	grid_size_count_.y = clamp(grid_size_count_.y, 1, 10000);

    ar.closeBlock();

	Color3c backgroundColor(backgroundColor_);
	ar.serialize(backgroundColor, "backgroundColor", "Цвет фона интерфейса");
	backgroundColor_ = Color4c(backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);

	Color3c workspaceColor(workspaceColor_);
	ar.serialize((Color3c&)(workspaceColor), "workspaceColor", "Цвет рабочей области");
	workspaceColor_ = Color4c(workspaceColor.r, workspaceColor.g, workspaceColor.b, 255);

	ar.serialize(ruler_width_, "ruler_width_", 0);
	ar.serialize(guides_, "guides_", 0);
    ar.serialize(zoom_mode_, "zoom_mode", 0);

	ar.serialize(debugShowEnabled, "debugShowEnabled", "Дебаговая инфа");
	ar.serialize(showDebugInterface, "showDebugInterface", "Опции дебага интерфейса");
	ar.serialize(showDebugEffects, "showDebugEffects", "Опции дебага эффектов");
}

void Options::Guide::serialize(Archive& ar)
{
	ar.serialize(position, "position", 0);
	ar.serialize(type, "type", 0);
}


Vect2f Options::calculateRelativeGridSize() const
{
	Vect2f resolution(UI_Render::instance().windowPosition().size());
	switch(grid_metrics_){
	case GRID_METRICS_RELATIVE:
		return grid_size_;
	case GRID_METRICS_PIXELS:
		return Vect2f(float(grid_size_pixels_.x) / float(resolution.x), float(grid_size_pixels_.y) / float(resolution.y));
	case GRID_METRICS_COUNT:
		return Vect2f(1.0f / float(grid_size_count_.x), 1.0f / float(grid_size_count_.y));
	default:
		xassert(0);
		return Vect2f::ID;
	}
}

WRAP_LIBRARY(Options, "Options", "Options", "Scripts\\Content\\UIEditorPreferences", 0, false);
