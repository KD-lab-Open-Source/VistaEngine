#include "StdAfx.h"
#include "RigidBodyBox.h"
#include "BaseUniverseObject.h"
#include "AbnormalStateAttribute.h"
#include "EffectController.h"
#include "CrashSystem.h"
#include "RenderObjects.h"
#include "NormalMap.h"
#include "Render\3dx\Simply3dx.h"
#include "Render\src\Scene.h"

CrashBody::CrashBody(cSimply3dx* model, float mass, ExplodeProperty* property_) : 
	effectController(0),
	property(property_),
	model_(model)
{
	effectController.setOwner(this);
	if(mass > FLT_EPS){
		RigidBodyPrmReference rigidBodyPrm("Debris");
		body = safe_cast<RigidBodyBox*>(RigidBodyBase::buildRigidBody(rigidBodyPrm->rigidBodyType, rigidBodyPrm, Vect3f::ZERO, Vect3f::ID, mass));
		body->setParameters(property_->z_gravity, property_->friction, property_->restitution);
		TriangleInfo triangleInfo;
		model_->GetTriangleInfo(triangleInfo, TIF_POSITIONS|TIF_ZERO_POS);
		body->buildGeomMesh(triangleInfo.positions, true);
	}else
		body = 0;
	BaseUniverseObject::radius_ = 0;
	attachSmart(model_);
}

bool CrashBody::evolve(float dt)
{
	if(body && body->evolve(dt)){
		BaseUniverseObject::setPose(body->pose(), false);
		streamLogicInterpolator.set(fSe3fInterpolation, model()) << body->posePrev() << body->pose();
	}
	effectController.logicQuant();
	return true;
}

void CrashBody::initPose( const Se3f& pose)
{
	BaseUniverseObject::setPose(pose, true);
	if(body)
		body->initPose(pose);
}

void CrashBody::updateRegion( float x1, float y1, float x2, float y2 )
{
	body->updateRegion(x1, y1, x2, y2);
}

void CrashBody::setOpacity(float opacity)
{
	if(body && body->onWater()){
		Vect3f centre(body->centreOfGravity());
		if(environment->water()->GetZFast(centre.xi(), centre.yi()) - centre.z > FLT_EPS)
			return;
	}
	streamLogicCommand.set(fCommandSimplyOpacity, model()) << opacity;
}

CrashBody::~CrashBody()
{
	delete body;
	effectController.release();
	RELEASE(model_);
}

CrashModel3dx::CrashModel3dx(const Vect3f& deltaPos, vector<cSimply3dx*>& debrises, const ExplodeProperty& property_, const EffectAttribute* effect, float explodeFactor, float liveTime, bool enableFantomMode) :
	enableFantomMode_(enableFantomMode),
	explodeFactor_(explodeFactor),
	property(property_),
	liveTime_(liveTime),
	liveTimeMax_(liveTime)
{
	vector<cSimply3dx*>::iterator iDebris;
	FOR_EACH(debrises, iDebris){
		cSimply3dx* debris = *iDebris;
		debris->setAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
		string name = debris->GetNodeName(0);
		int weight = (int)name[name.size() - 1] - (int)'0';
		xassert("Веса для осколков не заданы или введена лишняя нода" && weight <= 9 && weight >= 0);
		CrashBody* body(new CrashBody(debris, weight ? 10000.0f : 0.0f, &property));
		Se3f pose = debris->GetPositionSe();
		pose.trans() += deltaPos;
		body->initPose(pose);
		body->createEffect(effect);
		if(body->rigidBody())
			body->rigidBody()->setAngularVelocity(Vect3f(logicRNDfrnd(1.f), logicRNDfrnd(1.f), logicRNDfrnd(1.f)));
		crashBodyes.push_back(body);
		bodyWeights.push_back(weight);
	}
}

CrashModel3dx::~CrashModel3dx()
{
	vector<CrashBody*>::iterator it;
	FOR_EACH(crashBodyes, it)
		delete *it;
}

void CrashModel3dx::evolve( float dt )
{
	liveTime_ -= dt;
	vector<CrashBody*>::iterator crashBody;
	if(!enableFantomMode_ && liveTime_ <= liveTimeMax_ / 2.0f){
		float opacity(2.0f * liveTime_ / liveTimeMax_);
		FOR_EACH(crashBodyes, crashBody)
			(*crashBody)->setOpacity(opacity);
	}
	if(liveTime_ < 0 && (liveTime_ + dt) >= 0){
		FOR_EACH(crashBodyes, crashBody){
			CrashBody* body(*crashBody);
			body->removeEffect();
			if(body->rigidBody())
				body->rigidBody()->sleep();
		}
		return;
	}
	FOR_EACH(crashBodyes, crashBody)
		(*crashBody)->evolve(dt);
}

void CrashModel3dx::setVelocity(Vect3f velocity)
{
	vector<CrashBody*>::iterator crashBody;
	FOR_EACH(crashBodyes, crashBody){
		CrashBody* body(*crashBody);
		if(body->rigidBody())
			body->rigidBody()->setVelocity(velocity);
	}
}

void CrashModel3dx::addExplode( Vect3f point, float power, const Vect3f& explodeFactors)
{
	vector<CrashBody*>::iterator crashBody;
	vector<int>::iterator bodyWeight(bodyWeights.begin());
	FOR_EACH(crashBodyes, crashBody){
		CrashBody* body(*crashBody);
		if(body->rigidBody()){
			Vect3f temp(body->rigidBody()->centreOfGravity());
			temp.sub(point);
			temp.normalize(power * explodeFactor_ * (*bodyWeight) * body->rigidBody()->core()->mass());
			temp.mult(explodeFactors);
			body->rigidBody()->addImpulse(temp, Vect3f::ZERO);
		}
		++bodyWeight;
	}
}

void CrashModel3dx::updateRegion( float x1, float y1, float x2, float y2 )
{
	vector<CrashBody*>::iterator crashBody;
	FOR_EACH(crashBodyes, crashBody){
		CrashBody* body(*crashBody);
		if(body->rigidBody())
			body->updateRegion(x1, y1, x2, y2);
	}
}

void CrashModel3dx::showDebugInfo()
{
	vector<CrashBody*>::iterator crashBody;
	FOR_EACH(crashBodyes, crashBody)
		if((*crashBody)->rigidBody())
			(*crashBody)->rigidBody()->show();
}

void CrashSystem::addCrashModel(const DeathAttribute& deathAttribute, const c3dx* model, const Vect3f& position, const Vect3f& point, float power, float liveTime, const Vect3f& velocity)
{
	vector<cSimply3dx*> debrieses;
	if(!terScene->CreateDebrisesDetached(model, debrieses))
		return;
	CrashModel3dx* cm = new CrashModel3dx(position - model->GetPosition().trans(), debrieses, *deathAttribute.explodeReference, &deathAttribute.effectAttr, deathAttribute.explodeFactor, liveTime, deathAttribute.enableExplodeFantom);
	cm->setVelocity(velocity);
	Vect3f pose = position;
	if(fabsf(deathAttribute.explodeReference->selfPower) > FLT_EPS){
		Vect3f explodePos(pose.x + logicRNDfrnd(model->GetBoundRadius()/2.0f), pose.y + logicRNDfrnd(model->GetBoundRadius()/2.0f), float(vMap.getApproxAlt(pose.xi(),pose.yi())));
		cm->addExplode(explodePos, deathAttribute.explodeReference->selfPower, deathAttribute.explodeReference->explodeFactors);
	}
	if(point != Vect3f::ZERO && fabsf(power) > FLT_EPS)
		cm->addExplode(point, power, Vect3f::ID);
	crashModels.push_back(cm);
}

void CrashSystem::moveQuant(float dt)
{
	start_timer_auto();
	
	SwapVector<CrashModel3dx*>::iterator ci;
	for(ci = crashModels.begin(); ci != crashModels.end();) {
		(*ci)->evolve(dt);
		if((*ci)->dead()) {
			delete (*ci);
			ci = crashModels.erase(ci);
		} else 
			ci++;
	}
}

void CrashSystem::updateRegion( float x1, float y1, float x2, float y2 )
{
	SwapVector<CrashModel3dx*>::iterator ci;
	FOR_EACH(crashModels, ci)
		(*ci)->updateRegion(x1,y1,x2,y2);
}

CrashSystem::~CrashSystem()
{
	SwapVector<CrashModel3dx*>::iterator ci;
	FOR_EACH(crashModels, ci)
		delete (*ci);
}

void CrashSystem::showDebugInfo()
{
	SwapVector<CrashModel3dx*>::iterator ci;
	FOR_EACH(crashModels, ci)
		(*ci)->showDebugInfo();
}
