#ifndef __COMMAND_EDITOR_H_INCLUDED__
#define __COMMAND_EDITOR_H_INCLUDED__

#include "kdw\Viewport2D.h"
#include "kdw\Plug.h"
#include "kdw/CheckBox.h"
#include "kdw\Win32/Handle.h"
#include "Units\CommandsQueue.h"

struct Color4c;

class CommandEditorPlug;

class CommandEditorViewPort : public kdw::Viewport2D
{
public:
	enum { SIZE_X = 50, SIZE_Y = 50, BORDER = 3 };

	CommandEditorViewPort(CommandsQueue& value);

	void selectCellUnderPoint(const Vect2f& point);

	void onMouseMove(const Vect2i& delta);
	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);
	void onKeyDown(const sKey& key);

	void onRedraw(HDC dc);

	void onCommandCellDelete();
	void onMenuAdd(int index);

	void setPatternName(const char* name);
	CommandsQueue& value() const { return value_; }
    int selectedCommand() const;

	void setShowTypeNames(bool showTypeNames) { showTypeNames_ = showTypeNames; };
	bool showTypeNames() const{ return showTypeNames_; }

	sigslot::signal0& signalChanged(){ return signalChanged_; }

private:
	bool showTypeNames_;
	int selected_command_;
	Vect2f createPosition_;

	CommandsQueue& value_;
	sigslot::signal0 signalChanged_;

	ComboStrings comboStrings_;
};

struct ActorsSerializer
{
	ActorsSerializer(CommandsQueue::Actors& actors) : actors_(actors) {}

	void serialize(Archive& ar){
		ar.serialize(actors_, "actors", "Актеры");
	}

	CommandsQueue::Actors& actors_;
};

class CommandEditor : public kdw::VBox{
public:
	CommandEditor(CommandEditorPlug* plug);
	void onSet();

protected:
	void onShowTypeNames();
	CommandsQueue& value();
	void onChange();

	CommandEditorViewPort* viewport_;
	kdw::PropertyTree* propertyTree_;
	kdw::CheckBox* showTypeNames_;
	CommandEditorPlug* plug_;
	ActorsSerializer actorSerializer_;
};

class CommandEditorPlug: public kdw::Plug<CommandsQueue, CommandEditorPlug>{
public:
	CommandEditorPlug();
	static std::string valueAsString(const CommandsQueue& value){
		return value.c_str();
	}
	void onSet();
	kdw::Widget* asWidget();
protected:
	ShareHandle<CommandEditor> editor_;
};

#endif
