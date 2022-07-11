#include "StdAfx.h"
#include "FileUtils.h"
#include "ShadowWrapper.h"

void ShadowWrapper::addRef(c3dx* model)
{
	string modelFileName = normalizePath(model->GetFileName());
	ModelShadowWithRef& modelShadow = modelShadows_[modelFileName];
	if(modelShadow.addRef()){
		if(started)
			modelShadow.setShadowType(model);
		else
			modelShadow.setModel(model);
	}
}

void ShadowWrapper::decRef(c3dx* model)
{
	if(!model)
		return;
	string modelFileName = normalizePath(model->GetFileName());
	modelShadows_[modelFileName].decRef();
}

void ShadowWrapper::setForModel(c3dx* model, ShadowType shadowType, float shadowRadiusRelative)
{
	string modelFileName = normalizePath(model->GetFileName());
	modelShadows_[modelFileName].set(shadowType, shadowRadiusRelative);
}

void ShadowWrapper::serializeForModel(Archive& ar, c3dx* model, float factor)
{
	string modelFileName = normalizePath(model->GetFileName());
	ModelShadowWithRef& modelShadow = modelShadows_[modelFileName];
	modelShadow.serializeForEditor(ar, factor);
	if(ar.isOutput())
		modelShadow.setShadowType(model);
}

void ShadowWrapper::serialize(Archive& ar)
{
	if(ar.isOutput()){
		ModelShadows::iterator iShadow;
		for(iShadow = modelShadows_.begin(); iShadow < modelShadows_.end(); ){
			if(iShadow->second.active())
				++iShadow;
			else
				iShadow = modelShadows_.erase(iShadow);
		}
	}
	ar.serialize(modelShadows_, "modelShadows", 0);
	if(ar.isInput()){
		xassert(!started);
		ModelShadows::iterator iShadow;
		FOR_EACH(modelShadows_, iShadow)
			iShadow->second.apply();
		started = true;
	}
}