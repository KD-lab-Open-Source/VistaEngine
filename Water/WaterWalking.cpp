#include "stdafx.h"

#include "Environment\Environment.h"
#include "WaterWalking.h"
#include "Water.h"
#include "RenderObjects.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\TexLibrary.h"
#include "Render\Src\Scene.h"

#include "Units\Squad.h"

#include "Serialization\ResourceSelector.h"

cWaterPlume::cWaterPlume(UnitReal* unit, const WaterPlumeAttribute& attribute) : BaseGraphObject(0), frequency(attribute.waterPlumeFrequency)
{
	rate = 0.0f;
	textureCyrcle = GetTexLibrary()->GetElement3D(attribute.waterPlumeTextureName.c_str());
	pose.rot() = Mat3f::ID;
	pose.trans() = unit->position();
	radius = unit->radius();
	cObject3dx* obj = unit->modelLogic();
	PlumeNodes::const_iterator it;
	AttributeLegionary* atribute = (AttributeLegionary*)&unit->attr();
	FOR_EACH(atribute->plumeNodes, it){
		int i = it->node();
		if(i!=-1){
			UnitNode node;
			node.position = unit->modelLogicNodePosition(i).trans();
			node.radius = it->radius();
			node.index = i;
			nodes_.push_back(node);
		}
	}
	if(nodes_.empty()){
		int n = obj->GetNodeNum();
		for(int i=0; i<n;i++)
			if(strstr("step", obj->GetNodeName(i))){
				UnitNode node;
				node.position = unit->modelLogicNodePosition(i).trans();
				node.index = i;
				nodes_.push_back(node);
			}
		if(nodes_.empty()){
			UnitNode node;
			node.position = unit->modelLogicNodePosition(0).trans();
			node.index = 0;
			nodes_.push_back(node);
			verifyZ_ = false;
		}
		else
			verifyZ_ = true;
		float rad = obj->GetBoundRadius() / nodes_.size();
		NodeContainer::iterator it;
		FOR_EACH(nodes_, it)
			it->radius = rad;
	}
	if(atribute->sprayEffect.get()){
		NodeContainer::iterator it;
		FOR_EACH(nodes_, it){
			it->effect = terScene->CreateEffectDetached(*atribute->sprayEffect->getEffect(atribute->sprayEffectScale), NULL, false);
			it->effect->setCycled(true);
			it->effect->SetParticleRate(0);
			it->effect->SetFunctorGetZ(environment->water()->GetFunctorZ());
			it->onWater = atribute->sprayEffectOnWater;
			attachSmart(it->effect);
		}
	}
	xassert(!nodes_.empty());
}

cWaterPlume::UnitNode::UnitNode() : 
	index(0), 
	position(Vect3f::ZERO), 
	radius (0), 
	distance(0), 
	delta(0), 
	effect(NULL), 
	onWater(false) 
{
}

cWaterPlume::UnitNode::~UnitNode()
{
	if(effect)
		effect->StopAndReleaseAfterEnd();
}

cWaterPlume::~cWaterPlume()
{
	RELEASE(textureCyrcle);
}

void cWaterPlume::PreDraw(Camera* camera)
{
	if(camera->TestVisible(pose.trans(), radius))
		camera->Attach(SCENENODE_OBJECTSORT,this);
}

void cWaterPlume::UnitNode::update(const Vect3f& pos, bool enable, float waterZ, ParticlesUnDir& unDirPartls)
{
	Vect2f dir(position.x - pos.x, position.y - pos.y);
	position = pos;
	if(enable){
		if(effect)
			delta = 0;
	}else{
		delta = dir.norm();
		if(effect){
			Vect3f effectPos(pos);
			if(onWater)
				effectPos.z = waterZ;
			if(delta > 1e-3f){
				dir /= delta;
				effect->SetPosition(MatXf(Mat3f(-dir.y, -dir.x, 0, dir.x, -dir.y, 0, 0, 0, 1), effectPos));
			}else
				effect->SetPosition(MatXf(effect->GetPosition().rot(), effectPos));
		}

		distance += delta;
		if(distance > radius * 0.8f){
			ParticleUnDir circle; 
			circle.max_size = radius;
			circle.phase = 0.3f;
			circle.pos.set(pos.x, pos.y, waterZ);
			unDirPartls.push_front(circle);
			distance = 0;
		}
	}
}

void cWaterPlume::Animate(float dt)
{
	if(dt < 1.0f)
		return;
	dt_ = dt * 1e-3f;
	float rateTemp = rate / dt_;
	NodeContainer::iterator nd;
	FOR_EACH(nodes_, nd)
		if(nd->effect)
			nd->effect->SetParticleRate(min(nd->delta * rateTemp, 1.0f));
}

void cWaterPlume::Draw(Camera* camera)
{
	cInterfaceRenderDevice* rd = gb_RenderDevice;
	static Color4c color(255, 255, 255);
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, textureCyrcle);
	cQuadBuffer<sVertexXYZDT1>* pBuf = rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	ParticlesUnDir::iterator it;
	FOR_EACH(unDirPartls_, it){
		if (it->phase <= 1){
			sVertexXYZDT1* v = pBuf->Get();
			float size = it->phase * it->max_size;
			color.a = unsigned char((1 - it->phase) * 255);
			Vect3f sx(Vect3f::I); sx *= size;
			Vect3f sy(Vect3f::J); sy *= size;
			v[0].pos = it->pos - sx - sy; v[0].diffuse = color; v[0].GetTexel().set(0, 0);	
			v[1].pos = it->pos - sx + sy; v[1].diffuse = color; v[1].GetTexel().set(0, 1);
			v[2].pos = it->pos + sx - sy; v[2].diffuse = color; v[2].GetTexel().set(1, 0);
			v[3].pos = it->pos + sx + sy; v[3].diffuse = color; v[3].GetTexel().set(1, 1);
			it->phase += dt_ * frequency;
		}else{
			unDirPartls_.resize(it - unDirPartls_.begin());
			break;
		}
	}
	pBuf->EndDraw();
}
