#include "StdAfx.h"
#include "kdw/Plug.h"
#include "UserInterface/UI_Types.h"
#include "Serialization/SerializationFactory.h"
#include "UISpriteEditorViewport.h"
#include "kdw/Dialog.h"
#include "kdw/CheckBox.h"
#include "Render\Src\Texture.h"

class UISpriteEditor;
class UISpriteEditorPlug: public kdw::Plug<UI_Sprite, UISpriteEditorPlug>{
public:
	UISpriteEditorPlug();
	static std::string valueAsString(const UI_Sprite& value){
		if(value.texture())
			return value.texture()->name();
		else
			return "";
	}
	void onSet();
	kdw::Widget* asWidget();
protected:
	ShareHandle<UISpriteEditor> editor_;
};

class UISpriteEditor : public kdw::HBox{
public:
	UISpriteEditor(UISpriteEditorPlug* plug);
	void onSet(){
		viewport_->setSprite(value());
		if(propertyTree_)
			propertyTree_->revert();
	}
protected:
	void onPlayCheck();
	void updateButtons();
	void onChange();
	UI_Sprite& value(){ return plug_->value(); }

	UISpriteEditorViewport* viewport_;
	kdw::PropertyTree* propertyTree_;
	kdw::CheckBox* playCheck_;
	kdw::Button* prevButton_;
	kdw::Button* nextButton_;
	UISpriteEditorPlug* plug_;
};

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list

UISpriteEditorPlug::UISpriteEditorPlug()
: editor_(new UISpriteEditor(this))
{
}

#pragma warning(pop)

void UISpriteEditorPlug::onSet()
{
	editor_->onSet(); 
}
kdw::Widget* UISpriteEditorPlug::asWidget()
{
	return editor_; 
}

void UISpriteEditor::onPlayCheck()
{	
	viewport_->enableAnimation(playCheck_->status());
}

void UISpriteEditor::updateButtons()
{
	bool animation = viewport_->getSprite().isAnimated();
	playCheck_->setSensitive(animation);
	prevButton_->setSensitive(animation);
	nextButton_->setSensitive(animation);
}

void UISpriteEditor::onChange()
{
	value() = viewport_->getSprite();
	propertyTree_->revert();
}

UISpriteEditor::UISpriteEditor(UISpriteEditorPlug* plug)
: kdw::HBox(8)
, plug_(plug)
{
	viewport_ = new UISpriteEditorViewport();
	viewport_->setSprite(value());
	add(viewport_, true, true, true);

	kdw::VBox* vbox = new kdw::VBox(4);
	add(vbox);
	{
		propertyTree_ = new kdw::PropertyTree();
		vbox->add(propertyTree_, true, true, true);
		propertyTree_->setRequestSize(Vect2i(250, 100));
		propertyTree_->setCompact(true);
		propertyTree_->attach(Serializer(*viewport_));
		propertyTree_->signalChanged().connect(this, &UISpriteEditor::onChange);
		viewport_->signalChanged().connect(this, &UISpriteEditor::onChange);
		viewport_->signalTextureChanged().connect(this, &UISpriteEditor::updateButtons);

		kdw::HBox* buttonBox = new kdw::HBox(4);
		vbox->add(buttonBox);
		{

			prevButton_ = new kdw::Button(" < ");
			buttonBox->add(prevButton_);
			prevButton_->signalPressed().connect(viewport_, &UISpriteEditorViewport::prevFrame);
			
			playCheck_ = new kdw::CheckBox("Play");
			buttonBox->add(playCheck_, true, true, true);
			playCheck_->setButtonLike(true);
			playCheck_->setStatus(true);
			playCheck_->signalChanged().connect(this, &UISpriteEditor::onPlayCheck);

			nextButton_ = new kdw::Button(" > ");
			buttonBox->add(nextButton_);
			nextButton_->signalPressed().connect(viewport_, &UISpriteEditorViewport::nextFrame);
			
			updateButtons();
		}
	}
}

DECLARE_SEGMENT(UISpriteEditor)
REGISTER_PLUG(UI_Sprite, UISpriteEditorPlug)
