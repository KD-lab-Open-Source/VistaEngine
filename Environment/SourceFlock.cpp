#include "StdAfx.h"
#include <functional>
#include "SourceFlock.h"
#include "Serialization\Serialization.h"
#include "Serialization\RangedWrapper.h"
#include "Physics\NormalMap.h"
#include "Game\RenderObjects.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\EnumDescriptor.h"
#include "Render\Src\Scene.h"
#include "Render\3dx\Node3dx.h"

extern vrtMap vMap;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceFlock, FlockType, "FlockType")
REGISTER_ENUM_ENCLOSED(SourceFlock, FLOCK_BIRD, "Птички")
REGISTER_ENUM_ENCLOSED(SourceFlock, FLOCK_FISH, "Рыбки")
END_ENUM_DESCRIPTOR_ENCLOSED(SourceFlock, FlockType)

namespace {
	float rotationAngleBird = 0.015f;
	float rotationAngleFish = 0.07f;
	float ignoreAngleCos = 0.99f;
	float d_phase = 5.e-6f;
}

class Bird : public ShareHandleBase
{
public:
	Bird();
	~Bird() { xassert(!model_); }

	// положение без учета рельефа мира
	const Vect3f& position() const { return relativePose_.trans(); }
	const QuatF& orientation() const { return relativePose_.rot(); }
	// реальное положение модели на мире
	Vect3f realPosition() const { return Vect3f(relativePose_.trans().x, relativePose_.trans().y, realHeight_); }

	// передается направление полета
	void quant(const Vect3f& bestDir);

	void create(const SourceFlock* owner);
	void release();

	void setFlockness(float fl) { flockness_ = fl; }
	float flockness() const { return flockness_; }

	/// проверка на воду, возвращает границы воды в глобальных координатах.
	void checkWaterLevel(int x, int y, Rangef& level) const;

	//Vect3f savedcurrentdirection;
	//Vect3f d1;
	//Vect3f d2;
	//Vect3f d3;
	//Vect3f d4;

private:
	const SourceFlock* owner_;
	cObject3dx* model_;
	
	// точка, без учета облета
	Se3f relativePose_;
	// реальная высота на мире
	float realHeight_;

	// индивидуальная стадность
	float flockness_;
	
	// время с начала анимации
	float animationTime_;

	float boxDZ_;
	sBox6f box_;

	friend void fBirdInterpolation(XBuffer&);
	static void interpolate(XBuffer& stream);
};

Bird::Bird()
: model_(0)
, owner_(0)
, relativePose_(Se3f::ID)
, realHeight_(0.f)
, flockness_(0.5f)
, boxDZ_(0)
{
}

void Bird::quant(const Vect3f& direction)
{
	QuatF rot;
	Vect2f flatDirection(direction);
	flatDirection.normalize(1.f);
	
	xassert(model_);
	streamLogicInterpolator << model_;
	streamLogicInterpolator << relativePose_.rot() << realPosition();

	Vect3f dir(0,1,0);
	float cs = flatDirection.dot(Vect2f(relativePose_.rot().xform(dir))); // косинус угла поворота

	if(cs < ignoreAngleCos)
	{
		Vect3f sina(1,0,0);
		Vect3f norm(0, 0, flatDirection.dot(Vect2f(relativePose_.rot().xform(sina))) > 0.f ? -1.f : 1.f);

		if(owner_->flockType() == SourceFlock::FLOCK_BIRD)
			rot.set(rotationAngleBird, norm, 0);
		else
			rot.set(rotationAngleFish, norm, 0);
		rot.xform(dir);
		relativePose_.rot().postmult(rot);
	}

	dir.z = 0.05f * direction.z;
	
	bool hide = false;

	if(owner_->flockType() == SourceFlock::FLOCK_BIRD){
		relativePose_.trans().scaleAdd(dir, 2.f);
		realHeight_ = relativePose_.trans().z + normalMap->heightLinear(relativePose_.trans().x, relativePose_.trans().y);
	}
	else {
		relativePose_.trans().scaleAdd(dir, 4.f);

		Rangef waterLevel;
		Rangef waterLevel1;
		Rangef waterLevel2;
		Rangef waterLevel3;
		Rangef waterLevel4;
		Vect3f cp1(0, box_.ymax() + 10.0f, 0);
		Vect3f cp2(0, box_.ymin() - 10.0f, 0);
		Vect3f cp3(box_.xmax() + 10.0f, 0, 0);
		Vect3f cp4(box_.xmin() - 10.0f, 0, 0);
		relativePose_.xformPoint(cp1);
		relativePose_.xformPoint(cp2);
		relativePose_.xformPoint(cp3);
		relativePose_.xformPoint(cp4);
		checkWaterLevel(cp1.xi(), cp1.yi(), waterLevel1);
		checkWaterLevel(cp2.xi(), cp2.yi(), waterLevel2);
		checkWaterLevel(cp3.xi(), cp3.yi(), waterLevel3);
		checkWaterLevel(cp4.xi(), cp4.yi(), waterLevel4);
		waterLevel.setMaximum(min(min(waterLevel1.maximum(),waterLevel2.maximum()),min(waterLevel3.maximum(),waterLevel4.maximum())));
		waterLevel.setMinimum(max(max(waterLevel1.minimum(),waterLevel2.minimum()),max(waterLevel3.minimum(),waterLevel4.minimum())));
		realHeight_ = waterLevel.maximum() - relativePose_.trans().z;
		realHeight_ = waterLevel.clip(realHeight_);
		hide = waterLevel.maximum() < waterLevel.minimum();
	}

	streamLogicInterpolator << relativePose_.rot() << realPosition();

	if(owner_->hasAnimation()){
		streamLogicInterpolator << animationTime_ / owner_->animationTime();

		animationTime_ += logicPeriodSeconds;
		if(animationTime_ >= owner_->animationTime())
			animationTime_ -= owner_->animationTime();

		streamLogicInterpolator << animationTime_ / owner_->animationTime();
	}
	else
		streamLogicInterpolator << 0.f << 0.f;

	streamLogicInterpolator << hide;

	//savedcurrentdirection = dir;
}

inline void Bird::interpolate(XBuffer& stream)
{

	cObject3dx* model;
	stream.read(model);

	Se3f p[2];
	stream.read(p, sizeof(p));

	float ph[2];
	stream.read(ph, sizeof(ph));

	bool hide;
	stream.read(hide);

	if(hide)
		model->setAttribute(ATTRUNKOBJ_IGNORE);
	else {
		
		model->clearAttribute(ATTRUNKOBJ_IGNORE);

		Se3f s;
		s.interpolate(p[0], p[1], StreamInterpolator::factor());
		model->SetPosition(s);

		static float eps1=1+FLT_EPS;
		float phase = cycle(ph[0] + getDist(ph[1], ph[0], eps1)*StreamInterpolator::factor(), eps1);

		model->SetAnimationGroupPhase(0, phase);
	}
}


void Bird::create(const SourceFlock* owner)
{
	xassert(!model_);
	owner_ = owner;
	Vect3f position(
		owner->position().x + logicRNDfrnd(owner->radius() / 1.3f),
		owner->position().y + logicRNDfrnd(owner->radius() / 1.3f),
		logicRNDfabsRndInterval(owner->height().minimum() + 0.1f * owner->height().length(), owner->height().maximum() - 0.1f * owner->height().length()));
	
	relativePose_.set(QuatF::ID, position);
	realHeight_ = relativePose_.trans().z + normalMap->heightLinear(relativePose_.trans().x, relativePose_.trans().y);

	model_ = terScene->CreateObject3dxDetached(owner->modelName());
	xassert(model_);

	model_->SetScale(logicRNDinterval(owner->modelSize().minimum(), owner->modelSize().maximum()));
    	
	model_->SetPosition(Se3f(relativePose_.rot(), realPosition()));
	if(owner->hasAnimation()){
		model_->SetChain(owner->animationName());
		animationTime_ = logicRNDfabsRndInterval(0.f, owner->animationTime());
	}	

	model_->GetBoundBox(box_);
	boxDZ_ = box_.zmax() - box_.zmin();

	attachSmart(model_);
}

void Bird::release()
{
	RELEASE(model_);
}

void Bird::checkWaterLevel( int x, int y, Rangef& level ) const
{
	x = clamp(x, 0, vMap.H_SIZE - 1);
	y = clamp(y, 0, vMap.V_SIZE - 1);

	float ground = normalMap->heightLinear(x,y);

	level.setMaximum(ground + environment->water()->GetRelativeZ(x >> environment->water()->grid_shift, y >> environment->water()->grid_shift) - boxDZ_);
	level.setMinimum(ground + boxDZ_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SourceFlock::SourceFlock()
: flockCount_(10)
, height_(60.f, 120.f)
, size_(0.9f, 1.1f)
, animationPeriod_(-1.f)
, flockness_(0.5f)
, flockType_(FLOCK_BIRD)
{
}

SourceFlock::SourceFlock(const SourceFlock& src)
: flockCount_(src.flockCount_)
, modelName_(src.modelName_)
, animationName_(src.animationName_.comboList(), src.animationName_.value().c_str())
, animationPeriod_(src.animationPeriod_)
, flockness_(src.flockness_)
, flockType_(src.flockType_)
{
	height_ = src.height_;
	size_ = src.size_;
}

SourceFlock* SourceFlock::clone() const
{
	return new SourceFlock(*this);
}

SourceFlock::~SourceFlock()
{
}

void SourceFlock::setComboList()
{
	if(modelName_.empty())
		return;

	if(cObject3dx* model = terScene->CreateObject3dx(modelName_.c_str())){
		string comboList;
		int number = model->GetChainNumber();
		for(int i = 0; i < number; i++){
			comboList += "|";
			comboList += model->GetChain(i)->name;
		}
		animationName_.setComboList(comboList.c_str());
		model->Release();
	}
}

void SourceFlock::serialize(Archive& ar)
{
	__super::serialize(ar);

	static ModelSelector::Options options("*.3dx", "RESOURCE\\Models", "Will select location of 3DX model");
	ar.serialize(ModelSelector(modelName_, options), "modelFilename", "Модель");
	if(ar.isEdit() && ar.isOutput())
		setComboList();
	ar.serialize(animationName_, "animationName", "Анимационная цепочка");
	ar.serialize(size_, "size", "Масштаб модели");

	ar.serialize(flockType_, "flockType", "Тип стаи");
	ar.serialize(flockCount_, "flockCount_", "Количество особей");

	if(flockType_ == FLOCK_BIRD) {
		ar.serialize(height_, "height", "Высота над землей");
		ar.serialize(RangedWrapperf(flockness_, 0.f, 1.f) , "flockness", "Стадность");
	}
	else
		ar.serialize(height_, "height", "Глубина под водой");


	serializationApply(ar);
}

void SourceFlock::start()
{
	animationPeriod_ = -1.f;

	if(!modelName_.empty())
		if(cObject3dx* model = terScene->CreateObject3dx(modelName_.c_str())){
			int chInd = model->GetChainIndex(animationName_);
			animationPeriod_ = chInd >= 0 ? model->GetChain(chInd)->time : 0.f;
			model->Release();
		}

	knots_.resize(KNOTS_NUM);
	setPose(Se3f(orientation(), Vect3f(position2D(), vMap.initialHeight + height().center())), true);
	regenerate();
}

void SourceFlock::stop()
{
	release();
}


void SourceFlock::release()
{
	for_each(birds_.begin(), birds_.end(), mem_fun(&Bird::release));
	birds_.clear();
}

void SourceFlock::regenerate()
{
	// Инициализируем узловые точки
	Knots::iterator it;
	FOR_EACH(knots_, it)
		it->regenerate(this);

	// убиваем старых птиц
	release();
	
	if(!hasModel())
		return;

	//создаем новых
	for(int i = 0; i < flockCount_; ++i){
		birds_.push_back(new Bird());
		birds_.back()->create(this);
		birds_.back()->setFlockness(flockness_ < 0.1f ? 0.2f :
									flockness_ < 0.4f ? 0.1f :
									flockness_ > 0.9f ? 0.2f :
									(0.5f - (flockness_ - 0.4f) * logicRNDfabsRndInterval(0.7f, 1.1f) * 0.6));
	}
}

void SourceFlock::CenterNode::regenerate(const SourceFlock* owner)
{
	int a, b, c, d;
	do{
		a = logicRNDinterval(2, 7);
		c = logicRNDinterval(2, 7);
		do{
			b = 1 + logicRNDinterval(1, 3) << 1;
			d = 1 + logicRNDinterval(1, 3) << 1;
		}while(b == d);
	}while(a == b || c == d);
	int s1, s2;
	do{
		s1 = logicRNDfrand() > .5 ? 1: -1;
		s2 = logicRNDfrand() > .5 ? 1: -1;
	}while (s1 == s2);
	a_ = s1 * a;
	b_ = b;
	c_ = s2 * c;
	d_ = d;
	position_.z = logicRNDfabsRndInterval(owner->height().minimum() + 0.1f*owner->height().length(), owner->height().maximum() - 0.1f*owner->height().length());
}

Vect2f SourceFlock::CenterNode::getDelta() const
{
	return Vect2f(sinf(a_ * angle_) + cosf(b_ * angle_), sinf(c_ * angle_) + cosf(d_ * angle_));
}

void fBirdInterpolation(XBuffer& stream)
{
	start_timer_auto();
	unsigned size;
	stream.read(size);
	for(unsigned bn = 0; bn < size; ++bn)
		Bird::interpolate(stream);
}

void SourceFlock::quant()
{
	__super::quant();

	if(!active() || !hasModel())
		return;

	start_timer_auto();

	// Двигаем узловые точки
	float da = d_phase * (1000.f - clamp(radius(), 300.f, 600.f));
	Knots::iterator knot;
	FOR_EACH(knots_, knot){
		knot->angle_ += logicRNDfabsRnd(da);
		if(knot->angle_ > 2.f * M_PI){
			knot->angle_ -= 2.f * M_PI;
			knot->regenerate(this);
		}
		Vect2f delta(knot->getDelta());
		delta *= radius() / 2.5;
		knot->position_.set(position().x + delta.x, position().y + delta.y, knot->position_.z);
	}

	float _shiftSpeed = flockness_ < 0.5f ? 2.f : 1.f;
	Vect3f chaos(-sinf(knots_[0].angle_*_shiftSpeed*2.0), cosf(knots_[0].angle_*_shiftSpeed*3.0), 0.f);
	
	float _chaosCoeff =	flockness_ < 0.1f ? 0.1f :
						flockness_ <= 0.5f ? 0.05f :
						(0.1f + (flockness_ - 0.5f) * 0.4f);

	chaos.normalize(_chaosCoeff);

	streamLogicInterpolator.set(fBirdInterpolation) << birds_.size();

	Birds::iterator it;
	FOR_EACH(birds_, it){
		Bird* bird = *it;
		const Vect3f& birdPose = bird->position();
		
		Vect3f mainDir = Vect3f::ZERO;
		FOR_EACH(knots_, knot){
			Vect3f d2;
			d2.sub(knot->position_, birdPose);
			d2 /= d2.norm2();
			mainDir += d2;
			
			++knot;
			d2.sub(birdPose, knot->position_);
			d2 /= d2.norm2();
			mainDir += d2;
		}
		mainDir.normalize(bird->flockness());

		//bird->d1 = mainDir;

		mainDir += chaos;

		//bird->d3 = chaos;

		//bird->d2 = Vect3f::ZERO;

		if(birdPose.distance2(position()) > sqr(0.8f * radius()) || !height_.include(birdPose.z))
		{
			Vect3f correctionDir;
			correctionDir.sub(position(), birdPose);
			correctionDir.normalize();
			mainDir += correctionDir;
			
			//bird->d2 = correctionDir;
		}
		

		//bird->d4 = mainDir;

		bird->quant(mainDir);
	}
}

void SourceFlock::showEditor() const
{
	__super::showEditor();

	float crossSize = 2.0f;
	Color4c color = Color4c::GREEN;

	Birds::const_iterator bit;
	FOR_EACH(birds_, bit){
		Vect3f pos = (*bit)->realPosition();
		gb_RenderDevice->DrawLine(pos - Vect3f::I * crossSize, pos + Vect3f::I * crossSize, color);
		gb_RenderDevice->DrawLine(pos - Vect3f::J * crossSize, pos + Vect3f::J * crossSize, color);
		gb_RenderDevice->DrawLine(pos - Vect3f::K * crossSize, pos + Vect3f::K * crossSize, color);
		
		Vect3f dir(0.f, 1.f, 0.f);
		(*bit)->orientation().xform(dir);

		gb_RenderDevice->DrawLine(pos, pos + 15.f * dir, color);
	}

	crossSize = 10.0f;
	bool polus = true;
	Knots::const_iterator kit;
	FOR_EACH(knots_, kit){
		Vect3f pos = kit->position_;
		gb_RenderDevice->DrawLine(pos - Vect3f::I * crossSize, pos + Vect3f::I * crossSize, polus ? Color4c::RED : Color4c::GREEN);
		gb_RenderDevice->DrawLine(pos - Vect3f::J * crossSize, pos + Vect3f::J * crossSize, polus ? Color4c::RED : Color4c::GREEN);
		gb_RenderDevice->DrawLine(pos - Vect3f::K * crossSize, pos + Vect3f::K * crossSize, polus ? Color4c::RED : Color4c::GREEN);
		polus = !polus;
	}
}

void SourceFlock::showDebug() const
{
	float crossSize = 2.0f;
	Color4c color = Color4c::GREEN;

	Birds::const_iterator bit;
	FOR_EACH(birds_, bit){
		Vect3f pos = (*bit)->realPosition();
		show_vector(pos - Vect3f::I * crossSize, 2.f * Vect3f::I * crossSize, color);
		show_vector(pos - Vect3f::J * crossSize, 2.f * Vect3f::J * crossSize, color);
		show_vector(pos - Vect3f::K * crossSize, 2.f * Vect3f::K * crossSize, color);

		Vect3f dir(0.f, 1.f, 0.f);
		(*bit)->orientation().xform(dir);
		show_vector(pos, 15.f * dir, color);
	}

	crossSize = 10.0f;
	bool polus = true;
	Knots::const_iterator kit;
	FOR_EACH(knots_, kit){
		Vect3f pos = kit->position_;
		show_vector(pos - Vect3f::I * crossSize, 2.f * Vect3f::I * crossSize, polus ? Color4c::RED : color);
		show_vector(pos - Vect3f::J * crossSize, 2.f * Vect3f::J * crossSize, polus ? Color4c::RED : color);
		show_vector(pos - Vect3f::K * crossSize, 2.f * Vect3f::K * crossSize, polus ? Color4c::RED : color);
		polus = !polus;
	}
}
