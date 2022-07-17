#include "StdAfx.h"
#include "Universe.h"
#include "EnvironmentSimple.h"
#include "RenderObjects.h"
#include "TransparentTracking.h"
#include "..\Environment\Environment.h"
#include "EditorVisual.h"
#include "..\physics\WindMap.h"
#include "..\physics\crash\CrashSystem.h"

#include "EnvironmentSimple.h"
#include "EditArchive.h"
#include "CameraManager.h"

// _VISTA_ENGINE_EXTERNAL_ - нужно для переврода external-версии

REGISTER_CLASS(UnitBase, UnitEnvironmentSimple, "UnitEnvironmentSimple")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ENVIRONMENT_SIMPLE, UnitEnvironmentSimple);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UnitEnvironmentSimple, TreeMode, "TreeMode")
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, TREE_NORMAL, "TREE_NORMAL");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, TREE_FALLING, "TREE_FALLING");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, TREE_GROWING, "TREE_GROWING");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, TREE_FLOATING, "TREE_FLOATING");
END_ENUM_DESCRIPTOR_ENCLOSED(UnitEnvironmentSimple, TreeMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UnitEnvironmentSimple, TreeType, "TreeType")
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, FALL_AND_DISAPPEAR, "Падает и исчезает");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, FALL_AND_LIE, "Падает и лежит на земле");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, FALL_AND_LIE_OR_FLOAT, "Падает и лежит или плавает");
REGISTER_ENUM_ENCLOSED(UnitEnvironmentSimple, FALL_AND_GROW, "Падает, исчезает и вырастает заново");
END_ENUM_DESCRIPTOR_ENCLOSED(UnitEnvironmentSimple, TreeType)

UnitEnvironmentSimple::UnitEnvironmentSimple(const UnitTemplate& data)
: UnitEnvironment(data)
, fallLeavesEnabled_(false)
{
	modelSimple_ = 0;

	modelBurnt_ = false;
	treeMode_ = TREE_NORMAL;
	treeType_ = FALL_AND_DISAPPEAR;

	initialPose_ = Se3f::ID;
	destructibleFence_ = false;
	waterWeight_ = 0;
	stoneMass_ = 1.0f;
	springDamping3DX_ = 0;
}

UnitEnvironmentSimple::~UnitEnvironmentSimple()
{
	environment->shadowWrapper().decRef(modelSimple_);
	RELEASE(modelSimple_);
}

void UnitEnvironmentSimple::setPose(const Se3f& poseIn, bool initPose)
{
	Se3f posePrev = pose();
	__super::setPose(poseIn, initPose);

	if(initPose){
		initialPose_ = pose();	
		posePrev = pose();
	}
	
	if(modelSimple())
		streamLogicInterpolator.set(fSe3fInterpolation, modelSimple()) << posePrev << pose();
}

void UnitEnvironmentSimple::showEditor()
{
	__super::showEditor();

	if(modelSimple()){
		if(!editorVisual().isVisible(objectClass()))
			modelSimple()->SetAttr(ATTRUNKOBJ_IGNORE);
		else
			if(environment->isVisibleUnderForOfWar(this))
				modelSimple()->ClearAttr(ATTRUNKOBJ_IGNORE);
	}

	// Конверсия из симпл
	if(!dead() && !isEnvironmentSimple(environmentType_)){
		UnitBase* unit = player()->buildUnit(AuxAttributeReference(AUX_ATTRIBUTE_ENVIRONMENT));
		EditArchive ea;
		static_cast<EditOArchive&>(ea).serialize(*this, 0, 0);
		Kill();
		ea.setRootNode(ea.rootNode());
		static_cast<EditIArchive&>(ea).serialize(*unit, 0, 0);
		return;
	}
}

void UnitEnvironmentSimple::Quant()
{
	start_timer_auto();

	if(rigidBody_ && !rigidBody_->asleep() && treeMode_ == TREE_FALLING && !rigidBody_->isBox())
		enableBoxMode();

	__super::Quant();

	if(rigidBody_ && rigidBody_->asleep() && rigidBody_->isBox() && treeMode_ != TREE_FLOATING)
		stopFall();

	if(cameraManager->isVisible(position()))
		universe()->addVisibleUnit(this);
    
	if((burnt_ && !modelBurnt_) || (!burnt_ && modelBurnt_)){
		float scalePrev = modelSimple()->GetScale();
		setModel(modelName());
		if(modelSimple_)
			streamLogicCommand.set(fCommandSetScale, modelSimple_) << scalePrev;
	}

	if(destroyInWater_ && rigidBody_ && !isUnderEditor() && rigidBody_->onWater && 
		(treeMode_ == TREE_NORMAL || treeMode_ == TREE_GROWING)){
		explode();
		switch(treeType_){
		case FALL_AND_LIE_OR_FLOAT:
			treeMode_ = TREE_FLOATING;
		case FALL_AND_LIE:
		case FALL_AND_DISAPPEAR:
		case FALL_AND_GROW:
			startFall(Vect3f(position().x + logicRNDfrnd(1.f), position().y + logicRNDfrnd(1.f), position().z));
			break;
		default:
			Kill();
		}
	}

	switch(environmentType_){
	case ENVIRONMENT_TREE:
	case ENVIRONMENT_FENCE:
	case ENVIRONMENT_FENCE2:
		treeQuant();
		break;
	}

	fowQuant();

}

void UnitEnvironmentSimple::treeQuant()
{
	switch(treeMode_){

	case TREE_FLOATING:
		if(!rigidBody_->onWater)
			treeMode_ = TREE_FALLING;
		//rigidBody()->setDefaultAreaAnalize();
		//rigidBody_->awake();
		break;

	case TREE_NORMAL:
		if(rigidBody_){
			if(rigidBody_->isBox()){
				if(!destructibleFence_)
					explode();
				fallTimer_.start(GlobalAttributes::instance().treeLyingTime * 1000);
				if(environmentType_ == ENVIRONMENT_FENCE || environmentType_ == ENVIRONMENT_FENCE2 ){
					if(!destructibleFence_){
						environmentType_ = ENVIRONMENT_PHANTOM;
						setCollisionGroup(COLLISION_GROUP_GROUND_COLLIDER);
						setUnitAttackClass((AttackClass)environmentType_);
						treeMode_ = TREE_FALLING;
					}
					else if(rigidBody_->rotation().zcol().z < 0){
						explode();
						Kill();
					}
				}
				else
					treeMode_ = TREE_FALLING;
			}
		}
		break;
	case TREE_FALLING:
		if(treeType_ == FALL_AND_LIE_OR_FLOAT && rigidBody_->onWater){
			treeMode_ = TREE_FLOATING;
			if(!rigidBody_->isBox())
				enableBoxMode();
		}
		if(treeType_ == FALL_AND_GROW || treeType_ == FALL_AND_DISAPPEAR)
			setOpacity(0.002f * fallTimer_() / GlobalAttributes::instance().treeLyingTime);
		if(!fallTimer_){
			if(treeType_ == FALL_AND_GROW)
				treeRebirth();
			else if(treeType_ == FALL_AND_DISAPPEAR)
				Kill();
		}
		break;
	case TREE_GROWING:
		if(rigidBody_->isBox()){
			fallTimer_.start(GlobalAttributes::instance().treeLyingTime * 1000);
			treeMode_ = TREE_FALLING;
		}
		if(!burnt_){
			float t = growthTimer_();
			if(t > 0.99f){
				t = 1;
				treeMode_ = TREE_NORMAL;
			}
			modelSimple_->SetScale(t*scale_);
		}
		break;
	}
}

void UnitEnvironmentSimple::treeRebirth()
{
	if(destroyInWater_ && rigidBody_ && rigidBody_->onWater)
		return;

	setOpacity(1.0f);
	treeMode_ = TREE_GROWING;
	burnt_ = false;
	setPose(initialPose_, true);
	if(rigidBody_)
		stopFall();
	streamLogicCommand.set(fCommandSetScale, modelSimple_) << 0;
	growthTimer_.start(60000);
}

void UnitEnvironmentSimple::serialize(Archive& ar) 
{
	if(environmentType_ == ENVIRONMENT_TREE){
		ar.serialize(fallLeavesEnabled_, "enableFallingLeaves", "Включить падающие листья");
		if(ar.isEdit() && environment->fallLeaves())
			environment->fallLeaves()->serializeForModel(ar, modelName());
	}
	else
		ar.serialize(fallLeavesEnabled_, "enableFallingLeaves", 0);

	bool fenceFalling(false);
	if(environmentType_ == ENVIRONMENT_PHANTOM){
		fenceFalling = treeMode_ == TREE_FALLING;
		ar.serialize(fenceFalling, "fenceFalling", "Упавший забор");
		if(fenceFalling){
			if(treeMode_ == TREE_FALLING)
				fenceFalling = false;
			treeMode_ = TREE_FALLING;
		}else
			treeMode_ = TREE_NORMAL;
	}else
		ar.serialize(treeMode_, "treeMode", 0);

	__super::serialize(ar);

	if(!alive())
		return;

	if(ar.isEdit() && fenceFalling){
		treeMode_ = TREE_NORMAL;
		startFall(Vect3f::ZERO);
		treeMode_ = TREE_FALLING;
	}

	ar.serialize(waterWeight_, "waterWeight", "Коэффициент воздействия течения воды [0..100]");
	if(environmentType_ == ENVIRONMENT_TREE)
		ar.serialize(treeType_, "treeType", "Тип взаимодействия дерева");

	if(environmentType_ == ENVIRONMENT_FENCE || environmentType_ == ENVIRONMENT_FENCE2)	
		ar.serialize(destructibleFence_, "destructibleFence_", "Разрушаемый забор");

	if(environmentType_ == ENVIRONMENT_STONE)	
		ar.serialize(stoneMass_, "stoneMass_", "Масса камня");

	if((environmentType_ == ENVIRONMENT_FENCE || environmentType_ == ENVIRONMENT_FENCE2) && destructibleFence_) 
		deathParameters_.serializeEnvironment(ar);
	else{
		deathParameters_.serializeAbnormalStateEffects(ar);
		if(environmentType_ == ENVIRONMENT_TREE || environmentType_ == ENVIRONMENT_FENCE || environmentType_ == ENVIRONMENT_FENCE2)
			deathParameters_.serializeSources(ar);
	}

	if(treeType_ == FALL_AND_GROW)
		ar.serialize(initialPose_, "initialPose", 0);
	
	if(treeMode_ == TREE_FLOATING && !rigidBody_->isBox())
		enableBoxMode();

	if(!modelSimple())
		Kill(); 

}

void fCommandReleaseSpringDamping3DX(XBuffer& stream)
{
	SpringDamping3DX* springDamping;
	stream.read(springDamping);
	delete springDamping;
}

void UnitEnvironmentSimple::releaseSpringDamping3DX()
{
	if(springDamping3DX_){
		streamLogicCommand.set(fCommandReleaseSpringDamping3DX) << springDamping3DX_;
		springDamping3DX_ = 0;
	}
}

void UnitEnvironmentSimple::setModel(const char* name)
{
	if(fallLeavesHandle_.isValid() && environment->fallLeaves())
		environment->fallLeaves()->deleteHandle(fallLeavesHandle_);

	if(modelName_ != name || modelBurnt_ != burnt_ || !modelSimple_){
		if(!strlen(name))
			return;

		if(modelSimple()){
			releaseSpringDamping3DX();
			environment->shadowWrapper().decRef(modelSimple_);
			streamLogicCommand.set(fCommandRelease, modelSimple_);
		}

		cSimply3dx* modelIn;
		if(!burnt_)
			modelIn = terScene->CreateSimply3dxDetached(name);
		else
			modelIn = terScene->CreateSimply3dxDetached(name, "burnt");
		
		if(!modelIn){
			xassert_s(!"Модель или группа видимости не найденна", name);
			modelName_ = name;
			Kill();
			return;
		}
		
		modelSimple_ = modelIn;
		environment->shadowWrapper().addRef(modelSimple_);
		modelSimple_->SetPosition(MatXf(pose()));
//		SetLodDistance(modelSimple_,lodDistance_);

		attachSmart(modelSimple_);

		if(SpringDamping3DX::getWindNodesCount(modelSimple_))
			springDamping3DX_ = new SpringDamping3DX(modelSimple_);

		modelBurnt_ = burnt_;
	}

	if(name && strlen(name))
		modelName_ = name;

	sBox6f boundBox;
	cObject3dx* modelLogic = terScene->CreateLogic3dx(modelName_.c_str());
	if(modelLogic){
		modelLogic->GetBoundBox(boundBox);
		modelLogic->Release();
	}
	else{
		modelSimple_->GetBoundBoxUnscaled(boundBox);
	}

	if(radius() < FLT_EPS){
		radius_ = ((Vect2f&)boundBox.max).distance(boundBox.min)/2;
		return;
	}

	scale_ = radius()/max(((Vect2f&)boundBox.max).distance(boundBox.min)/2, 0.001f);
	height_ = (boundBox.max.z - boundBox.min.z)*scale_;
	
	streamLogicCommand.set(fCommandSetScale, modelSimple_) << scale_;
		
	if((environmentType_ != ENVIRONMENT_PHANTOM || treeMode_ == TREE_FALLING) && radius_ > 0.001f){
		if(environmentType_ != ENVIRONMENT_PHANTOM)
			setCollisionGroup(COLLISION_GROUP_COLLIDER);
		else
			setCollisionGroup(COLLISION_GROUP_GROUND_COLLIDER);

		string referenceName;
		if(checkGround_){
			switch (environmentType_){
			case ENVIRONMENT_TREE:
				if(rigidBody_ && rigidBody_->isBox())
					referenceName = "Environment Tree Falling";
				else
					referenceName = "Environment Tree";
				break;

			case ENVIRONMENT_FENCE:
			case ENVIRONMENT_FENCE2:
			case ENVIRONMENT_PHANTOM:
				if(rigidBody_ && rigidBody_->isBox())
					referenceName = "Environment Fence Falling";
				else
					referenceName = "Environment Fence";
				break;

			case ENVIRONMENT_STONE:
				if(isUnderEditor())
					referenceName = "Environment Stone Editor";
				else
					referenceName = "Environment Stone";
				break;

			default:
				referenceName = "Environment";
			}
		} else {
			referenceName = "Environment Phantom";
		}
		RigidBodyPrmReference rigidBodyPrm(referenceName.c_str());

		if(!rigidBody_ || rigidBody_->prm().rigidBodyType != rigidBodyPrm->rigidBodyType) {
			delete rigidBody_;
			rigidBody_ = RigidBodyBase::buildRigidBody(rigidBodyPrm->rigidBodyType, rigidBodyPrm, scale_*boundBox.min, scale_*boundBox.max, stoneMass_);
//			if(environmentType_ == ENVIRONMENT_TREE)
//				rigidBody()->setPointAreaAnalize();
		}
		else
			rigidBody_->build(*rigidBodyPrm, scale_*boundBox.min, scale_*boundBox.max, stoneMass_);
		rigidBody_->setPose(pose());
		if(rigidBody_->isBox() && treeType_ == FALL_AND_LIE_OR_FLOAT) {
			safe_cast<RigidBodyBox*>(rigidBody_)->setWaterAnalysis(true); 
			safe_cast<RigidBodyBox*>(rigidBody_)->setWaterWeight(waterWeight_ / 100.0f);
			safe_cast<RigidBodyBox*>(rigidBody_)->setWaterLevel(attr().waterLevel);
		}
		if(checkGroundPoint_)
			rigidBody()->setPointAreaAnalize();
		if(holdOrientation_)
			rigidBody()->setHoldOrientation(verticalOrientation_);
	} else {
		if(rigidBody_) {
			delete rigidBody_;
			rigidBody_ = 0;
		}
	}

	if(environmentType_ == ENVIRONMENT_STONE){
		TriangleInfo triangleInfo;
		modelSimple()->GetTriangleInfo(triangleInfo, TIF_POSITIONS|TIF_ZERO_POS);
		vector<Vect3f>::iterator vertex;
		float modelScale(modelSimple()->GetScale());
		if(fabsf(modelScale - scale_) > FLT_EPS){
			modelScale = scale_ / modelScale;
			FOR_EACH(triangleInfo.positions, vertex)
				vertex->scale(modelScale);
		}
		xassert(rigidBody_->isBox());
		safe_cast<RigidBodyBox*>(rigidBody_)->buildGeomMesh(triangleInfo.positions, false);
	}

	if(environmentType_ == ENVIRONMENT_TREE){
		if(!burnt_ && fallLeavesEnabled_)
			fallLeavesHandle_ = environment->fallLeaves()->createHandle(modelSimple_);
	}

	if(hideByDistance)
		modelSimple()->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);
	SetLodDistance(modelSimple(),lodDistance_);

/*
	if(lighted_)
		model()->ClearAttr(ATTRUNKOBJ_NOLIGHT);
	else
		model()->SetAttr(ATTRUNKOBJ_NOLIGHT);

	if(hideByDistance)
		model()->SetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE);
*/
}

void UnitEnvironmentSimple::collision(UnitBase* p, const ContactInfo& contactInfo)
{
	if(environmentType_ & (ENVIRONMENT_BUSH | ENVIRONMENT_STONE | ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE))
		__super::collision(p, contactInfo);
	
	// Шевеление куста
	if(environmentType_ == ENVIRONMENT_BUSH  && springDamping3DX_ && p->rigidBody() && p->rigidBody()->isUnit() && safe_cast<RigidBodyUnit*>(p->rigidBody())->unitMove()){
		Vect3f direction(0, logicRNDinterval(-60, 60), 0);
		p->pose().xformVect(direction);
		springDamping3DX_->applyImpulse(direction);
	}

	if((environmentType_ & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2 | ENVIRONMENT_TREE))
		&& (p->attr().environmentDestruction & environmentType_)){
		startFall(p->position());
	}


	if(environmentType_ == ENVIRONMENT_STONE && (p->attr().environmentDestruction & environmentType_)){
		xassert(rigidBody_->isBox());
		Vect3f point = contactInfo.collisionPoint(this);
		Vect3f normal = contactInfo.collisionPoint(p);
		normal.sub(point);
		float penetration = normal.norm();
		normal.Normalize();
		safe_cast<RigidBodyBox*>(rigidBody_)->addContact(point, normal, penetration);
		rigidBody_->awake();
	}
}

void UnitEnvironmentSimple::Kill()
{
	releaseSpringDamping3DX();
		
	if(modelSimple())
		streamLogicPostCommand.set(fCommandSetIgnored, modelSimple()) << true;

	if(environment->fallLeaves())
		environment->fallLeaves()->deleteHandle(fallLeavesHandle_);

	__super::Kill();
}

void UnitEnvironmentSimple::explode()
{
	if(deathAttr().explodeReference->enableExplode && modelSimple())
		universe()->crashSystem->addCrashModel(deathAttr(), modelSimple(), lastContactPoint_, lastContactWeight_, GlobalAttributes::instance().debrisLyingTime);

	__super::explode();
}

void UnitEnvironmentSimple::setOpacity(float opacity) 
{ 
	streamLogicCommand.set(fCommandSimplyOpacity, modelSimple()) << opacity;
}

void UnitEnvironmentSimple::fowQuant()
{
/*
	cFogOfWar* fow = environment->fogOfWar();
	if(!fow || !model())
		return;

	switch(attr().fow_mode){
	case FVM_HISTORY_TRACK:
		switch(fow->GetFogState(position2D().xi(), position2D().yi())){
		case FOGST_NONE:
			model()->ClearAttr(ATTRUNKOBJ_IGNORE);
			break;
		case FOGST_HALF:
			if (!model()->GetAttr(ATTRUNKOBJ_IGNORE)){
				universe()->addFowModel(model());
				model()->SetAttr(ATTRUNKOBJ_IGNORE);
			}
			break;
		case FOGST_FULL:
			model()->SetAttr(ATTRUNKOBJ_IGNORE);
			break;
		}
		break;
	case FVM_NO_FOG:
		if (fow->GetFogState(position2D().xi(), position2D().yi())==FOGST_NONE)
			model()->ClearAttr(ATTRUNKOBJ_IGNORE);
		else
			model()->SetAttr(ATTRUNKOBJ_IGNORE);
		break;
	}
*/
}
bool UnitEnvironmentSimple::checkInPathTracking( const UnitBase* tracker ) const
{
	if(environmentType_ == ENVIRONMENT_STONE && stoneMass_ >= tracker->attr().mass)
		if(tracker->rigidBody() && tracker->rigidBody()->isUnit() && safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode())
			return false;
		else
			return true;

	return __super::checkInPathTracking(tracker);
}

void UnitEnvironmentSimple::graphQuant(float dt)
{
	start_timer_auto();
	__super::graphQuant(dt);

	if(SpringDamping3DX* springDamping = springDamping3DX_)
		springDamping->evolve(modelSimple_, windMap->getPerlin(position2D()) , dt);
	
}

void UnitEnvironmentSimple::showDebugInfo()
{
	__super::showDebugInfo();
	if(showDebugUnitEnvironment.treeType)
		show_text(position(), getEnumName(treeType_), CYAN);
	if(showDebugUnitEnvironment.treeMode)
		show_text(position(), getEnumName(treeMode_), CYAN);
}

void UnitEnvironmentSimple::mapUpdate( float x0, float y0, float x1, float y1 )
{
	__super::mapUpdate(x0, y0, x1, y1);
}

void UnitEnvironmentSimple::startFall(const Vect3f& point)
{
	if(!rigidBody_->isBox()){
		streamLogicCommand.set(fCommandBlowFallLeaves) << fallLeavesHandle_;

		if(environment->fallLeaves()){
			environment->fallLeaves()->setBlow(fallLeavesHandle_);
		}

		enableBoxMode();
		if(treeMode_ != TREE_FALLING){
			safe_cast<RigidBodyBox*>(rigidBody_)->startFall(point);
		}
	}
}

void UnitEnvironmentSimple::enableBoxMode()
{
	string referenceName;

	switch (environmentType_){
		case ENVIRONMENT_TREE:
		case ENVIRONMENT_BUSH:
			referenceName = "Environment Tree Falling";
			break;

		case ENVIRONMENT_FENCE:
		case ENVIRONMENT_FENCE2:
		case ENVIRONMENT_PHANTOM:
			referenceName = "Environment Fence Falling";
			break;

		case ENVIRONMENT_STONE:
			return;

		default:
			xxassert(0, "Неизвестный environmentType");
			return;
	}

	RigidBodyPrmReference rigidBodyPrm(referenceName.c_str());

	RigidBodyBase* rigidBodyNew = RigidBodyBase::buildRigidBody(rigidBodyPrm->rigidBodyType, rigidBodyPrm, rigidBody_->boxMin(), rigidBody_->boxMax(), stoneMass_);
	rigidBodyNew->initPose(rigidBody_->pose());
	
	delete rigidBody_;
	rigidBody_ = rigidBodyNew;
	rigidBody_->awake();
	if(treeType_ == FALL_AND_LIE_OR_FLOAT) {
		safe_cast<RigidBodyBox*>(rigidBody_)->setWaterAnalysis(true); 
		safe_cast<RigidBodyBox*>(rigidBody_)->setWaterWeight(waterWeight_ / 100.0f);
		safe_cast<RigidBodyBox*>(rigidBody_)->setWaterLevel(attr().waterLevel);
	}
}

void UnitEnvironmentSimple::stopFall()
{
	if((environmentType_ & (ENVIRONMENT_FENCE | ENVIRONMENT_FENCE2)) && destructibleFence_){
		explode();
		Kill();
		return;
	}

	string referenceName;

	switch (environmentType_){
		case ENVIRONMENT_TREE:
			referenceName = "Environment Tree";
			break;

		case ENVIRONMENT_FENCE:
		case ENVIRONMENT_FENCE2:
		case ENVIRONMENT_PHANTOM:
			referenceName = "Environment Fence";
			break;

		case ENVIRONMENT_BUSH:
			referenceName = "Environment";
			break;

		case ENVIRONMENT_STONE:
			return;

		default:
			xxassert(0, "Неизвестный environmentType");
			return;
	}

	RigidBodyPrmReference rigidBodyPrm(referenceName.c_str());

	RigidBodyBase* rigidBodyNew = RigidBodyBase::buildRigidBody(rigidBodyPrm->rigidBodyType, rigidBodyPrm, rigidBody_->boxMin(), rigidBody_->boxMax(), stoneMass_);
	rigidBodyNew->setPose(rigidBody_->pose());
	delete rigidBody_;
	rigidBody_ = rigidBodyNew;
}

bool UnitEnvironmentSimple::setAbnormalState( const AbnormalStateAttribute& state, UnitBase* ownerUnit)
{
	if(deathParameters_.abnormalStateEffect(state.type())){
		if(destroyInAbnormalState_){
			explode();
			Kill();
			return true; 
		}
		if(!modelBurnt_){
			burnt_ = true;
			float scalePrev = modelSimple()->GetScale();
			setModel(modelName());
			if(modelSimple_)
				streamLogicCommand.set(fCommandSetScale, modelSimple_) << scalePrev;
		}
	}

	return UnitBase::setAbnormalState(state, ownerUnit);
}
