#include "stdafx.h"
#include "CommandEditor.h"
#include "kdw\PopupMenu.h"
#include "kdw\ClassMenu.h"
#include "kdw\VSplitter.h"

DECLARE_SEGMENT(CommandEditor)
REGISTER_PLUG(CommandsQueue, CommandEditorPlug)

#pragma warning(push)
#pragma warning(disable: 4355)

CommandEditorPlug::CommandEditorPlug()
: editor_(new CommandEditor(this))
{
}

#pragma warning(pop)

void CommandEditorPlug::onSet()
{
	editor_->onSet();
}

kdw::Widget* CommandEditorPlug::asWidget()
{
	return editor_;
}

CommandEditor::CommandEditor(CommandEditorPlug* plug)
: kdw::VBox(8)
, plug_(plug)
, actorSerializer_(plug->value().actors)
{

	kdw::VSplitter* vspl = new kdw::VSplitter(1, 4);
	add(vspl, true, true, true);
	{
		kdw::VBox* vbox = new kdw::VBox(4);
		vspl->add(vbox, 0.75f);
		{
			viewport_ = new CommandEditorViewPort(value());
			viewport_->signalChanged().connect(this, &CommandEditor::onChange);
			vbox->add(viewport_, true, true, true);

			showTypeNames_ = new kdw::CheckBox("Show type names");
			showTypeNames_->setButtonLike(false);
			showTypeNames_->setStatus(false);
			showTypeNames_->signalChanged().connect(this, &CommandEditor::onShowTypeNames);
			vbox->add(showTypeNames_);
		}
		
		propertyTree_ = new kdw::PropertyTree();
		propertyTree_->setRequestSize(Vect2i(250, 100));
		propertyTree_->setCompact(false);
		//propertyTree_->attach(Serializeable(*viewport_));
		propertyTree_->signalChanged().connect(this, &CommandEditor::onChange);
		vspl->add(propertyTree_);

	}
}

void CommandEditor::onSet()
{
	if(propertyTree_)
		propertyTree_->attach(Serializer(actorSerializer_));
		//propertyTree_->detach();
		//propertyTree_->revert();
	viewport_->redraw();
}

void CommandEditor::onShowTypeNames()
{
	viewport_->setShowTypeNames(showTypeNames_->status());
	viewport_->redraw();
}

void CommandEditor::onChange()
{
	if(viewport_->selectedCommand() != -1)
		propertyTree_->attach(Serializer(viewport_->value()[viewport_->selectedCommand()]));
	else
		propertyTree_->attach(Serializer(actorSerializer_));
	viewport_->redraw();
}

CommandsQueue& CommandEditor::value()
{ 
	return plug_->value(); 
}

CommandEditorViewPort::CommandEditorViewPort(CommandsQueue& value)
: kdw::Viewport2D(0, 12) 
, selected_command_(-1)
, value_(value)
, createPosition_(Vect2f::ZERO)
{
	showTypeNames_ = false;

	comboStrings_.push_back(getEnumNameAlt(COMMAND_ID_TALK));
	comboStrings_.push_back(getEnumNameAlt(COMMAND_ID_UPGRADE));
}

void CommandEditorViewPort::onRedraw(HDC dc)
{
	__super::onRedraw(dc);

	Rectf rt = Rectf(0, -SIZE_Y, 1000, SIZE_Y*6);
	fillRectangle(dc, rt, Color4c::WHITE);

	for(float y = rt.top(); y <= rt.bottom(); y += SIZE_Y) 
		drawLine(dc, Vect2f(rt.left(), y), Vect2f(rt.right(), y), Color4c(128, 128, 128));

	fillRectangle(dc, Rectf(Vect2f(rt.left(), -1), Vect2f(rt.right(), 1)), Color4c::BLACK);

	Win32::StockSelector old_font(dc, positionFont_);
	for(int i = 0; i < value_.size(); i++){
		const UnitCommandExtended& command = value_[i];
		Vect2f size(SIZE_X, SIZE_Y);
		Vect2f selectionBorder(2, 2);
		Vect2f anchorBorder(4, 4);
		Vect2f border(6, 6);
		Vect2f pos = command.coordinates(i);
		if(i == selected_command_)
			fillRectangle(dc, Rectf(pos + selectionBorder, size - selectionBorder*2), Color4c::RED);
		if(strcmp(command.anchor().c_str(), ""))
			fillRectangle(dc, Rectf(pos + anchorBorder, size - anchorBorder*2), Color4c::BLUE);
		int key = command.commandID();
		Color4c color = CommandColorManager::instance().getColor(CommandID(key));
		fillRectangle(dc, Rectf(pos + border, size - border*2), color);
	
		XBuffer str;
		if(showTypeNames_){
			const EnumDescriptor& desc = getEnumDescriptor(CommandID(0));
			str < " " <= i + 1 < " (" < desc.nameAlt(key) < ")";
		}
		else
			str < " " <= i + 1 < " ";

		drawText(dc, Rectf(pos, size), str);
	}
}

int CommandEditorViewPort::selectedCommand() const
{
	return selected_command_;
}

void CommandEditorViewPort::onMouseButtonDown(kdw::MouseButton button)
{
	__super::onMouseButtonDown(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		selectCellUnderPoint(clickPoint());
		redraw();
	}
	else if(button == kdw::MOUSE_BUTTON_LEFT){
		selectCellUnderPoint(clickPoint());
		redraw();
	}
}

void CommandEditorViewPort::onMouseMove(const Vect2i& delta)
{
	__super::onMouseMove(delta);
}

void CommandEditorViewPort::selectCellUnderPoint(const Vect2f& v) 
{
	selected_command_ = -1;
	for(int i = 0; i < value_.size(); i++){
		Rectf rect(value_[i].coordinates(i), Vect2f(SIZE_X, SIZE_Y));
		if(rect.point_inside(v))
			selected_command_ = i;
	}
	signalChanged_.emit();
}

class CommandTypeClassItemAdder: public kdw::ClassMenuItemAdder{
public:
	CommandTypeClassItemAdder(CommandEditorViewPort* commandEditor)
		: commandEditor_(commandEditor)
	{
	}
	void operator()(kdw::PopupMenuItem& root, int index, const char* text){
		root.add(text, index)
			.connect(commandEditor_, &CommandEditorViewPort::onMenuAdd);

	}
protected:
	CommandEditorViewPort* commandEditor_;
};

void CommandEditorViewPort::onMouseButtonUp(kdw::MouseButton button)
{
	__super::onMouseButtonUp(button);

	if(button == kdw::MOUSE_BUTTON_RIGHT){
		if(!scrolling()){
			selectCellUnderPoint(clickPoint());

			if(selected_command_ == -1) {
				createPosition_ = clickPoint();
				kdw::PopupMenu menu(300);
				kdw::PopupMenuItem& addItem = menu.root().add(TRANSLATE("Добавить"));
				CommandTypeClassItemAdder(this).generateMenu(addItem, comboStrings_, false);
				menu.spawn(this);
			} 
			else{
				kdw::PopupMenu menu(300);
				menu.root().add(TRANSLATE("Удалить"))
					.connect(this, &CommandEditorViewPort::onCommandCellDelete)
					.setHotkey(sKey(VK_DELETE));
				menu.spawn(this);
			}
		}
	}
	if(button == kdw::MOUSE_BUTTON_LEFT){
	}
}

void CommandEditorViewPort::onKeyDown(const sKey& key)
{
	if(key.fullkey == VK_DELETE && selected_command_ != -1)
		onCommandCellDelete();
}

void CommandEditorViewPort::onMenuAdd(int index)
{
	xassert(index >= 0 && index < comboStrings_.size());
	if(selected_command_ == -1){
		const char* name = (comboStrings_.begin() + index)->c_str();
		const EnumDescriptor& desc = getEnumDescriptor(CommandID(0));
		int key = desc.keyByNameAlt(name);
		UnitCommandExtended command(CommandID(key), createPosition_.y > 0 ? createPosition_.y/SIZE_Y : -1);
		selected_command_ = clamp((int)createPosition_.x/SIZE_X, 0, (int)value_.size());
		value_.insert(value_.begin() + selected_command_, command);
		signalChanged_.emit();
	}
	redraw();
}

void CommandEditorViewPort::onCommandCellDelete()
{
	xassert(selected_command_ >= 0);
	if(selected_command_ >= 0) {
		value_.erase(value_.begin() + selected_command_);
		selected_command_ = -1;
		signalChanged_.emit();
		redraw();
	}
}

void CommandEditorViewPort::setPatternName(const char* name)
{
	value_.setName(name); 
}

Vect2f UnitCommandExtended::coordinates(int index) const
{
	return Vect2f(CommandEditorViewPort::SIZE_X*index, CommandEditorViewPort::SIZE_Y*actorID_);
}
