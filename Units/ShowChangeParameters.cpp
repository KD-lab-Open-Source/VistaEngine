#include "stdafx.h"
#include "Parameters.h"
#include "Environment\SourceManager.h"
#include "UnitInterface.h"

void ShowChangeParameterSetting::serialize(Archive& ar)
{
	ar.serialize(typeRef_, "typeRef", "Тип параметра");
	ar.serialize(changeSettings_, "changeSettings", "Визуализация изменения");
}

void ShowChangeParametersController::create(const UnitInterface* owner, const ParameterSet& showDelta, CreateType createType)
{
	clear();
	for(int idx = 0; idx < showDelta.size(); ++idx)
		if(const ShowChangeSettings* param = owner->attr().getShowChangeSettings(showDelta[idx].index))
			if(param->showFlyParameters)
				switch(createType){
					case SET:
						controllers_.push_back(make_pair(showDelta[idx].index, sourceManager->addShowChangeController(ShowChangeController(owner, param, 0, owner->attr().initialHeightUIParam))));
						break;
					case VALUES:
						controllers_.push_back(make_pair(showDelta[idx].index, sourceManager->addShowChangeController(ShowChangeController(owner, param, showDelta[idx].value, owner->attr().initialHeightUIParam))));
						break;
					case SHOW:
						controllers_.push_back(make_pair(showDelta[idx].index, sourceManager->addShowChangeController(ShowChangeController(owner, param, 0, owner->attr().initialHeightUIParam))));
						controllers_.back().second->update(showDelta[idx].value);
						break;
				}
}

void ShowChangeParametersController::setOwner(const UnitInterface* owner)
{
	ShowChangeControllers::iterator it;
	FOR_EACH(controllers_, it)
		it->second->setOwner(owner);
}

void ShowChangeParametersController::clear()
{
	setOwner(0);
	controllers_.clear();
}

void ShowChangeParametersController::add(const ParameterSet& addVal)
{
	ShowChangeControllers::iterator it;
	FOR_EACH(controllers_, it)
		it->second->update(it->second->value() + addVal.findByIndex(it->first, 0.f));
}

void ShowChangeParametersController::update(const ParameterSet& newVal)
{
	ShowChangeControllers::iterator it;
	FOR_EACH(controllers_, it)
		it->second->update(newVal.findByIndex(it->first, it->second->value()));
}