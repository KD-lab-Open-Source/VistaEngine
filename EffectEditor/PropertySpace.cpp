#include "StdAfx.h"
#include "PropertySpace.h"
#include "kdw/PropertyTree.h"
#include "kdw/Serialization.h"
#include "Serialization/SerializationFactory.h"
#include "kdw/CommandManager.h"
#include "kdw/Navigator.h"
#include "EffectDocument.h"

PropertySpace::PropertySpace()
{
	globalDocument->signalChanged().connect(this, &PropertySpace::onModelChanged);

	tree_ = new kdw::PropertyTree();
	tree_->signalChanged().connect(this, PropertySpace::onPropertyChanged);
	tree_->signalBeforeChange().connect(this, PropertySpace::onPropertyBeforeChange);
	tree_->setCompact(true);
	add(tree_);

	globalDocument->signalCurveChanged().connect(this, &PropertySpace::onCurveChanged);

	commands().get("property");
	setMenu("property");

	onModelChanged(0);
}

void PropertySpace::onCurveChanged(bool significantChange)
{
	//if(significantChange)
	tree_->update();
}

void PropertySpace::onPropertyChanged()
{
	globalDocument->freeze(this);
	if(attachedNode_)
		attachedNode_->setChanged(true);
	globalDocument->unfreeze(this);
}

void PropertySpace::onPropertyBeforeChange()
{
	if(globalDocument->activeEmitter())
		globalDocument->history().pushEmitter();
	else if(globalDocument->activeEffect())
		globalDocument->history().pushEffect();
}

void PropertySpace::onModelChanged(kdw::ModelObserver* changer)
{
	if(changer == this)
		return;

	attachedNode_ = kdw::Navigator::model()->findFirstSelectedNode();
	if(attachedNode_){
		Serializer ser = attachedNode_->serializer();
		if(ser){
			tree_->attach(ser);
			return;
		}
	}
	tree_->detach();
}
namespace kdw{
REGISTER_CLASS(Space, PropertySpace, "Свойства")
}
