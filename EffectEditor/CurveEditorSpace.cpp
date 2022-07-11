#include "StdAfx.h"
#include "CurveEditorSpace.h"
#include "kdw/CheckBox.h"
#include "kdw/Button.h"
#include "kdw/Serialization.h"
#include "Serialization/SerializationFactory.h"

#include "kdw/Toolbar.h"
#include "kdw/ImageStore.h"
#include "kdw/CurveEditor.h"
#include "kdw/GradientBar.h"
#include "kdw/CommandManager.h"

#include "EffectDocument.h"
#include "CurveEditorElements.h"

CurveEditorSpace::CurveEditorSpace()
{
	globalDocument->signalChanged().connect(this, &CurveEditorSpace::onModelChanged);
	globalDocument->signalCurveToggled().connect(this, &CurveEditorSpace::onCurveToggled);
	globalDocument->signalTimeChanged().connect(this, &CurveEditorSpace::onTimeChanged);

	xassert(globalDocument);

	kdw::VBox* box = new kdw::VBox();
	{
		curveEditor_ = new kdw::CurveEditor();
		curveEditor_->signalTimeChanged().connect(this, &CurveEditorSpace::onCurveEditorTimeChanged);
		box->add(curveEditor_, false, true, true);

		gradientColorBar_ = new kdw::GradientBar();
		gradientColorBar_->signalChanged().connect(this, &CurveEditorSpace::onGradientChanged);
		gradientColorBar_->setRequestSize(Vect2i(32, 32));
		gradientColorBar_->setHasAlpha(false);
		box->add(gradientColorBar_, false, false, false);

		gradientAlphaBar_ = new kdw::GradientBar();
		gradientAlphaBar_->setHasColor(false);
		gradientAlphaBar_->signalChanged().connect(this, &CurveEditorSpace::onGradientChanged);
		box->add(gradientAlphaBar_, false, false, false);
	}
	add(box);

	updateMenus();
	
	kdw::Toolbar* toolbar = new kdw::Toolbar(&commands());
	toolbar->setImageStore(new kdw::ImageStore(24, 24, "CURVE_EDITOR", RGB(255, 0, 255)));
	toolbar->addButton("curveEditor.playback.rewind", 0);
	toolbar->addButton("curveEditor.playback.pause", 1);
	addToHeader(toolbar);

	setMenu("curveEditor");

	update();
}

void CurveEditorSpace::onTimeChanged(kdw::ModelObserver* changer)
{
	if(changer != this){
		commands().get("curveEditor.playback.pause").check(globalDocument->pause());
		curveEditor_->setTime(globalDocument->time());
		curveEditor_->setDuration(globalDocument->duration());
	}
}

void CurveEditorSpace::onCurveEditorTimeChanged()
{
	globalDocument->setPause(true, this);
	if(curveEditor_->time() < globalDocument->time())
		globalDocument->queueSetTime(curveEditor_->time(), this);
	else
		globalDocument->setTime(curveEditor_->time(), this);
}

void CurveEditorSpace::onGradientChanged()
{
	globalDocument->freeze(this);
	if(NodeEmitter* nodeEmitter = globalDocument->activeEmitter()){
		EmitterKeyInterface* emitter = nodeEmitter->get();
		bool changed = false;
		if(KeysColor* gradient = emitter->gradientColor()){
			*gradient = gradientColorBar_->get();
			changed = true;
		}		
		if(KeysColor* gradient = emitter->gradientAlpha()){
			*gradient = gradientAlphaBar_->get();
			changed = true;
		}
		if(changed){
			nodeEmitter->buildKey();
			nodeEmitter->setChanged(true);
		}
	}
	globalDocument->unfreeze(this);
}

void CurveEditorSpace::updateMenus()
{
	commands().get("curveEditor.playback.pause").setAutoCheck(true)
		.connect(this, CurveEditorSpace::onMenuPlaybackPause);
	commands().get("curveEditor.playback.rewind")
		.connect(this, CurveEditorSpace::onMenuPlaybackRewind);

}

void CurveEditorSpace::onMenuPlaybackPause()
{
	bool pause = commands().get("curveEditor.playback.pause").checked();
	globalDocument->setPause(pause, this);
}

void CurveEditorSpace::onMenuPlaybackRewind()
{
	globalDocument->setTime(0.0f, this);
}

void CurveEditorSpace::update()
{
	curveEditor_->clear();

	if(globalDocument->activeEffect()){
		if(NodeEmitter* emitter = globalDocument->activeEmitter()){
			NodeCurve* activeCurve = globalDocument->activeCurve();
			int count = emitter->childrenCount();
			for(int i = 0; i < count; ++i){
				NodeCurve* curve = safe_cast<NodeCurve*>(emitter->child(i));
				bool active = curve == activeCurve;
				if(curve->visible() && !active)
					curveEditor_->addElement(new CurveCKey(curve, false));
			}
			// активная кривая добавляется последней
			if(activeCurve && activeCurve->visible())
				curveEditor_->addElement(new CurveCKey(activeCurve, true));

			// синяя полоска внизу
			if(emitter->get()->generationPointCount() > 1){
				curveEditor_->addElement(new GenerationTimeLineElement(emitter));
			}

			if(const KeysColor* gradient = emitter->get()->gradientColor()){
				gradientColorBar_->set(*gradient);
				gradientColorBar_->setAllowAddPoints(true);
			}
			else{
				gradientColorBar_->set(KeysColor());
				gradientColorBar_->setAllowAddPoints(false);
			}

			if(const KeysColor* gradient = emitter->get()->gradientAlpha()){
				gradientAlphaBar_->set(*gradient);
				gradientAlphaBar_->setAllowAddPoints(true);
			}
			else{
				gradientAlphaBar_->set(KeysColor());
				gradientAlphaBar_->setAllowAddPoints(false);
			}
		}
		else{
			curveEditor_->addElement(new EffectLifeTimeElement(globalDocument->activeEffect()));
			
			gradientColorBar_->set(KeysColor());
			gradientColorBar_->setAllowAddPoints(false);
			gradientAlphaBar_->set(KeysColor());
			gradientAlphaBar_->setAllowAddPoints(false);
		}

	}
}

void CurveEditorSpace::onSelectedChanged(kdw::ModelObserver* changer)
{

}

void CurveEditorSpace::onModelChanged(kdw::ModelObserver* changer)
{
	if(selectedNode_)
		signal_disconnect(&selectedNode_->signalChanged());
	selectedNode_ = globalDocument->findFirstSelectedNode();
	if(selectedNode_)
		selectedNode_->signalChanged().connect(this, &CurveEditorSpace::onSelectedChanged);

	update();
}

void CurveEditorSpace::onCurveToggled()
{
	update();
}

namespace kdw{
REGISTER_CLASS(Space, CurveEditorSpace, "Редактор кривых");
}
