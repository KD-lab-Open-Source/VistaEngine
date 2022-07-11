#include "StdAfx.h"
#include "kdw/PropertyRow.h"
#include "kdw/PropertyTreeModel.h"
#include "kdw/PropertyTree.h"
#include "UserInterface/UI_Types.h"
#include "Win32/Drawing.h"
#include "Win32/Window.h"
#include "Serialization/SerializationFactory.h"
#include "UISpriteEditorViewport.h"
#include "kdw/Dialog.h"
#include "kdw/CheckBox.h"

class UISpriteEditor : public kdw::HBox{
public:
	UISpriteEditor(const UI_Sprite& sprite);
	UI_Sprite sprite() const{ return viewport_->getSprite(); }
protected:
	void onPlayCheck();
	void updateButtons();

	UISpriteEditorViewport* viewport_;
	kdw::CheckBox* playCheck_;
	kdw::Button* prevButton_;
	kdw::Button* nextButton_;
};

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

UISpriteEditor::UISpriteEditor(const UI_Sprite& sprite)
: kdw::HBox(8)
{
	viewport_ = new UISpriteEditorViewport();
	viewport_->setSprite(sprite);
	add(viewport_, true, true, true);

	kdw::VBox* vbox = new kdw::VBox(4);
	add(vbox);
	{
		kdw::PropertyTree* propertyTree = new kdw::PropertyTree();
		vbox->add(propertyTree, true, true, true);
		propertyTree->setRequestSize(Vect2i(250, 100));
		propertyTree->setCompact(true);
		propertyTree->attach(Serializeable(*viewport_));
		viewport_->signalChanged().connect(propertyTree, &kdw::PropertyTree::revert);
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

class PropertyRowUISprite : public kdw::PropertyRowImpl<UI_Sprite, PropertyRowUISprite>{
public:
	PropertyRowUISprite(void* object, int size, const char* name, const char* nameAlt, const char* typeName)
	: kdw::PropertyRowImpl<UI_Sprite, PropertyRowUISprite>(object, size, name, nameAlt, typeName)
	{}

	PropertyRowUISprite() {}

	bool onActivateWidget(kdw::PropertyTree* tree, kdw::PropertyRow* hostRow){
		return onActivate(tree);
	}

	bool onActivate(kdw::PropertyTree* tree){
		kdw::Dialog dialog(tree);
		dialog.setTitle(TRANSLATE("Редактор спрайтов"));
		dialog.setDefaultSize(Vect2i(750, 550));
		dialog.addButton(TRANSLATE("OK"), kdw::RESPONSE_OK);
		dialog.addButton(TRANSLATE("Отмена"), kdw::RESPONSE_CANCEL);
		dialog.setResizeable(true);

		UISpriteEditor* editor = new UISpriteEditor(value());
		dialog.add(editor);
		
		if(dialog.showModal() == kdw::RESPONSE_OK){
			value() = editor->sprite();
			tree->model()->rowChanged(this);
		}
		return true;
	}

	std::string valueAsString() const{ 
		if(value().texture())
			return value().texture()->GetName();
		else
			return "";
	}
};

REGISTER_PROPERTY_ROW(UI_Sprite, PropertyRowUISprite);
namespace kdw{ REGISTER_CLASS(PropertyRow, PropertyRowUISprite, "UI_Sprite"); }
