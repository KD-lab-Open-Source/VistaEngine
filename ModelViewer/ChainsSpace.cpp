#include "StdAfx.h"
#include "ChainsSpace.h"

#include "kdw/Serialization.h"
#include "kdw/Label.h"
#include "kdw/ScrolledWindow.h"
#include "kdw/ComboBox.h"
#include "kdw/HBox.h"
#include "kdw/VBox.h"

#include "MainView.h"

#include "Render/3dx/Static3dxBase.h"
#include "Render/3dx/Node3dx.h"

class VisibilitySetComboBox : public kdw::ComboBox{
public:
	VisibilitySetComboBox(int setIndex)
	: set_(setIndex)
	{
		cObject3dx* model = globalDocument->model();
		cStaticVisibilitySet& set = model->GetVisibilitySet(VisibilitySetIndex(setIndex));
		int count = int(set.visibility_groups[0].size());
		for(int i = 0; i < count; ++i){
			cStaticVisibilityChainGroup* chainGroup = set.visibility_groups[0][i];
			const char* name = chainGroup->name.c_str();
			add(name);
		}
		update();
	}

	void update(){
		VisibilityGroupIndex visibilityGroup(set_);
		setSelectedIndex(visibilityGroup.igroup);
	}

	void onSelectionChanged(){
		int index = selectedIndex();
		model_->SetVisibilityGroup(VisibilityGroupIndex(index), VisibilitySetIndex(set_));
		globalDocument->updateChainDuration();
	}
protected:
	cObject3dx* model_;
	int set_;
};

//KDW_REGISTER_CLASS(Widget, VisibiliT, "Выпадающий список цепочек анимации");

class AnimationGroupComboBox : public kdw::ComboBox{
public:
	AnimationGroupComboBox(int group = 0)
	: animationGroup_(group)
	{
		setModel(globalDocument->model());
	}
	void onSelectionChanged(){
		int index = selectedIndex();
		if(animationGroup_ == -1){
			for(int i = 0; i < model_->GetAnimationGroupNumber(); ++i)
				model_->SetAnimationGroupChain(i, index);
		}
		else
			model_->SetAnimationGroupChain(animationGroup_, index);
		globalDocument->updateChainDuration();
		kdw::ComboBox::onSelectionChanged();
	}
	
	void update(){
		setModel(model_);
	}	
	void setModel(cObject3dx* model){
		setSensitive(model != 0);
		bool modelChanged = model_ != model;
		
		if(modelChanged || model == 0)
			clear();

		if(model_ = model){
			if(modelChanged){
				int count = model->GetChainNumber();
				for(int i = 0; i < count; ++i){
					cAnimationChain* chain = model->GetChain(i);
					const char* name = chain->name.c_str();
					add(name);
				}
			}
			int selectedIndex = (animationGroup_ == -1) ? model->GetAnimationGroupChain(0) : model->GetAnimationGroupChain(animationGroup_);
			setSelectedIndex(selectedIndex);
		}
	}

protected:
	cObject3dx* model_;
	int animationGroup_;
};

KDW_REGISTER_CLASS(Widget, AnimationGroupComboBox, "ModelViewer\\Выпадающий список цепочек анимации");

ChainsSpace::ChainsSpace()
{
	xassert(globalDocument);
	globalDocument->signalModelChanged().connect(this, &ChainsSpace::onModelChanged);

	kdw::VBox* vbox = new kdw::VBox(8, 8);
	add(vbox);
	{
		vbox->pack(new kdw::Label(TRANSLATE("Группы видимости:"), true, 0));
		kdw::ScrolledWindow* scrolledWindow = new kdw::ScrolledWindow(0);
		visibilityGroupsBox_ = new kdw::VBox(4, 0);

		{
			vbox->pack(new kdw::Label(TRANSLATE("Группы анимации:"), true, 0));
			vbox->pack(new kdw::Label(TRANSLATE("Цепочка для всех групп:"), false, 0));
			vbox->pack(chainsComboBox_ = new AnimationGroupComboBox(-1));
			
				animationGroupsBox_ = new kdw::VBox(4, 0);
				vbox->pack(animationGroupsBox_);
		}
	}
}

void ChainsSpace::onModelChanged()
{
	cObject3dx* model = globalDocument->model();
	if(model){
		visibilityGroupsBox_->clear();
		int numVisibilitySets = model->GetVisibilitySetNumber();
		
		for(int i = 0; i < numVisibilitySets; ++i){
			const char* name = model->GetVisibilitySetName(VisibilitySetIndex(i));

			kdw::Box* row = new kdw::HBox;
			visibilityGroupsBox_->pack(row);
			{
				row->pack(new kdw::Label(name), false, true, true);
				VisibilitySetComboBox* combo = new VisibilitySetComboBox(i);
				row->pack(combo, false, true, true);
			}
		}
		visibilityGroupsBox_->showAll();

		animationGroupsBox_->clear();
		
		chainsComboBox_->signalSelectionChanged().disconnect_all();
		chainsComboBox_->setModel(model);

		int numAnimationGroups = model->GetAnimationGroupNumber();
		for(int i = 0; i < numAnimationGroups; ++i){
			const char* name = model->GetAnimationGroupName(i);

			kdw::Box* row = new kdw::HBox;
			animationGroupsBox_->pack(row);
			{
				row->pack(new kdw::Label(name), false, true, true);
				AnimationGroupComboBox* combo = new AnimationGroupComboBox(i);
				chainsComboBox_->signalSelectionChanged().connect(combo, &AnimationGroupComboBox::update);
				row->pack(combo, false, true, true);
			}
		}
		animationGroupsBox_->showAll();
	}
}

void ChainsSpace::onLODChanged()
{
	/*
	MainView* view_ = 0;
	if(view_){
		view_->setLODLevel(lodCombo_->selectedIndex());
	}
	*/
}

KDW_REGISTER_CLASS(Space, ChainsSpace, "Цепочки и группы")
