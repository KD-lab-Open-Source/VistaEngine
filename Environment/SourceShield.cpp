#include "stdafx.h"
#include "SourceShield.h"

#include "Serialization.h"
#include "RangedWrapper.h"
#include "..\Environment\Environment.h"
#include "Player.h"
#include "..\Util\ResourceSelector.h"
#include "RenderObjects.h"

#include "Universe.h"

SourceShield::SourceShield()
{
	height_ = radius();
	centerDeep_ = 0;
	sphere_ = 0;
	//diffuseColor_.set(1, 1, 1, 0.6f);
	diffuseColor_.GetOrCreateKey(0)->Val().set(1, 1, 1, 1.0f);
	diffuseColor_.GetOrCreateKey(1)->Val().set(1, 1, 1, 1.0f);
	modelName_ = "scripts\\resource\\balmer\\shield.3dx";
	phase_ = 0;
	activateDTime_ = 0.05f;
	activateSpeed_ = 1;
	activatePhase_ = 0;
	deactivate_ = false;
}

SourceShield::SourceShield(const SourceShield& src)
: SourceDamage(src)
{
	height_ = src.height_;
	centerDeep_ = src.height_;
	diffuseColor_ = src.diffuseColor_;
	modelName_ = src.modelName_;
	sphere_ = 0;
	active_ = false;
	phase_ = src.phase_;
	activateDTime_ = src.activateDTime_;
	activateSpeed_ = src.activateSpeed_;
	activatePhase_ = 0;
	deactivate_ = false;
}

SourceShield::~SourceShield()
{
	RELEASE(sphere_);
}

void SourceShield::sphereInit()
{
	RELEASE(sphere_);

	extern class cScene* terScene;
	sphere_ = environment->scene()->CreateObject3dxDetached(modelName_.c_str());
	if(!sphere_){
		xxassert(false, "Не найдена модель защитного поля");
		return;
	}
	
	sColor4f clr = diffuseColor_.Get(0);
	sphere_->SetColorOld(&clr, &clr);
	sphere_->SetAttr(ATTRUNKOBJ_2PASS_ZBUFFER);
	sphere_->SetAttr(ATTRUNKOBJ_NOLIGHT);
	sphere_->ClearAttr(ATTRUNKOBJ_SHADOW);
	sphere_->ClearAttr(ATTRCAMERA_SHADOWMAP);

	setRadius(radius());
	sphere_->SetPosition(Se3f(orientation(), sphereCenter()));
	activatePhase_ = 0;
	activateDTime_ = logicTimePeriodInv/activateSpeed_*10;
	deactivate_ = false;
	attachSmart(sphere_);
}

void SourceShield::start()
{
	__super::start();
	sphereInit();
}
bool SourceShield::killRequest()
{
	if(activatePhase_ < 0)
		return true;

	deactivate_ = true;
	return false;
}

void SourceShield::stop()
{
	__super::stop();
	RELEASE(sphere_);
}
void SourceShield::setPose(const Se3f& pos, bool init)
{
	Se3f posePrev = init ? Se3f(pos.rot(), sphereCenter()) : Se3f(orientation(), sphereCenter());
	__super::setPose(pos, init);

	if(sphere_)
		streamLogicInterpolator.set(fSe3fInterpolation, sphere_) << posePrev << Se3f(pos.rot(), sphereCenter());
}

void SourceShield::setRadius(float _radius)
{
	Se3f posePrev = pose();
	__super::setRadius(_radius);

	height_ = clamp(height_, 1.f, _radius);
	centerDeep_ = (sqr(height_) + sqr(_radius)) / (2 * height_) - height_;

	if(sphere_){
		sBox6f box;
		sphere_->GetBoundBox(box);
		streamLogicCommand.set(fCommandSetScale, sphere_) << sphereRadius()*2/(box.max.x - box.min.x + 0.01f);
		streamLogicInterpolator.set(fSe3fInterpolation, sphere_) << posePrev << Se3f(orientation(), sphereCenter());
	}
}

void SourceShield::quant()
{
	__super::quant();

	if(!active() && !sphere_)
		return;

	xassert(sphere_);
	
	Player* player = universe()->activePlayer();
	const Vect3f* points = environmentPoints();
	if(player->fogOfWarMap() && points){
		bool isVisible = false;
		// WARNING: точек в enviornmentPoints всего ENVIRONMENT_POINTS_COUNT + 1
		const Vect3f* end = points + ENVIRONMENT_POINTS_COUNT + 1; 
		for(const Vect3f* it = points; it != end; ++it)
			if(player->fogOfWarMap()->isVisible(Vect2i(position2D() + Vect2f(*it)))){
				isVisible = true;
				break;
			}
		if(bool(!sphere_->GetAttr(ATTRUNKOBJ_IGNORE)) != isVisible)
			streamLogicCommand.set(fCommandSetIgnored, sphere_) << !isVisible;
	}

	phase_ +=0.01f;
	if (phase_>1.f)
		phase_ = 0;
	interpolatorPhase_ = phase_;

	if (deactivate_)
		activatePhase_ -= activateDTime_;
	else
		activatePhase_ = min(activatePhase_ + activateDTime_, 1.f);
	if (activatePhase_ < 0)	{
		kill();
		return;
	}

	sColor4f clr = diffuseColor_.Get(phase_);
	clr.a *= activatePhase_;
	interpolatorColor_ = sColorInterpolate(clr,clr,sColor4f(1,1,1,1));
	interpolatorColor_(sphere_);
	//sphere_->SetColor(&clr, &clr);
	interpolatorPhase_(sphere_, 0);
}
void SourceShield::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(height_, "height", "высота купола");
	

	ar.serialize(ModelSelector(modelName_, ModelSelector::BALMER3DX_OPTIONS), "shield_model", "Модель купола");

	ar.serialize(diffuseColor_, "diffuseColor", "Цвет купола");
	ar.serialize(activateSpeed_,"activateSpeed", "Скорость активации (сек.)");
	serializationApply(ar);
}

void SourceShield::showDebug() const
{
	__super::showDebug();

	for(float z_add = 0.95f * height_; z_add >= 0.f; z_add -= 0.9f * height_ / 6.f){
		float rad = sqrtf(sqr(sphereRadius()) - sqr(centerDeep_ + z_add));
		show_vector(Vect3f(position().x, position().y, position().z + z_add), rad, YELLOW);
	}
}

bool SourceShield::canApply(const UnitBase* target) const
{
	if(inZone(target->position()))
		return __super::canApply(target);

	return false;
}

namespace TraceShieldsHelpers{

typedef vector<const SourceShield*> Shields;

class ShieldScanOperator
{
public:
	enum Type{
		SCAN_ALL,
		SCAN_ENEMY,
		SCAN_OWN
	};
	
	ShieldScanOperator(const Player* player = 0, const Vect3f& point = Vect3f::ZERO, Type type = SCAN_OWN){
		reset(player, point, type);
	}
	void reset(const Player* player, const Vect3f& point, Type type){
		player_ = player;
		position_ = point;
		type_ = type;
		sources_.clear();
	}

	void operator()(SourceBase* src){
		if(src->type() == SOURCE_SHIELD && src->active()){
			const SourceShield* shield = safe_cast<const SourceShield*>(src);
			if(shield->inZone(position_)){
				switch(type_){
					case SCAN_OWN:
						if(player_->isEnemy(shield->player()))
							return;
						break;
					case SCAN_ENEMY:
						if(!player_->isEnemy(shield->player()))
							return;
					default:
						;
				};
				sources_.push_back(shield);
			}
		}
	}

	Shields& sources() { return sources_; }

private:

	const Player* player_;
	Vect3f position_;
	Type type_;
	Shields sources_;
};

class ShieldScanAllOperator
{
public:
	enum Type{
		SCAN_ALL,
		SCAN_ENEMY,
		SCAN_OWN
	};

	ShieldScanAllOperator(const Player* player = 0, Type type = SCAN_OWN){
		reset(player, type);
	}
	void reset(const Player* player, Type type){
		player_ = player;
		type_ = type;
		sources_.clear();
	}

	void operator()(SourceBase* src){
		if(src->type() == SOURCE_SHIELD && src->active()){
			const SourceShield* shield = safe_cast<const SourceShield*>(src);
			switch(type_){
				case SCAN_OWN:
					if(player_->isEnemy(shield->player()))
						return;
					break;
				case SCAN_ENEMY:
					if(!player_->isEnemy(shield->player()))
						return;
				default:
					;
			};
			sources_.push_back(shield);
		}
	}

	Shields& sources() { return sources_; }

private:

	const Player* player_;
	Type type_;
	Shields sources_;
};

// ищет ближайшее к началу отрезка пересечение
Vect3f crossLineSpheres(const Vect3f& st, const Vect3f& fin, const Shields& spheres, Vect3f* shieldCenter = 0)
{
	float t_cros = 2.f;
	
	float dx = fin.x - st.x;
	float dy = fin.y - st.y;
	float dz = fin.z - st.z;

	float A = sqr(dx) + sqr(dy) + sqr(dz);
	if(A < 0.001f)
		return Vect3f(-1.f, -1.f, -1.f);

	for(Shields::const_iterator it = spheres.begin(); it != spheres.end(); ++it){
		Vect3f centr = (*it)->sphereCenter();

		//At^2 + 2kt + C = 0;
		float cx = st.x - centr.x;
		float cy = st.y - centr.y;
		float cz = st.z - centr.z;

		float k = dx*cx + dy*cy + dz*cz;
		float C = sqr(cx) + sqr(cy) + sqr(cz) - sqr((*it)->sphereRadius());

		float D4 = sqr(k) - A*C;
		if(D4 < 0)
			continue;
		else
			D4 = sqrtf(D4);

		float t1 = (D4 - k) / A;
		float t2 = (-D4 - k) / A;

		if(t1 >= 0.f && (t1 < t2 || t2 < 0.f))
			t2 = t1;
		if(t2 >= 0.f && t2 < t_cros){
			t_cros = t2;
			if(shieldCenter)
				*shieldCenter = centr;
		}
	}
	
	if(t_cros > 1.f)
		return Vect3f(-1.f, -1.f, -1.f);

	return Vect3f(st.x + dx * t_cros, st.y + dy * t_cros, st.z + dz * t_cros);
}

inline void sort2(Shields& lst){
	int size = lst.size();
	if(size < 2)
		return;
	else if(size == 2){
		Shields::iterator it = lst.begin();
		if(*it < *(it+1))
			swap(*it, *(it+1));
	}
	else
		sort(lst.begin(), lst.end());
};

}; //namespace TraceShieldsHelpers

using namespace TraceShieldsHelpers;

bool SourceShield::traceShieldsCoherence(const Vect3f& point_start, const Vect3f& point_finish, const Player* owner, Vect3f* intersect)
{
	static ShieldScanOperator start;
	static ShieldScanOperator finish;
	
	if(owner){
		start.reset(owner, point_start, ShieldScanOperator::SCAN_ENEMY);
		finish.reset(owner, point_finish, ShieldScanOperator::SCAN_ENEMY);
	}
	else{
		start.reset(owner, point_start, ShieldScanOperator::SCAN_ALL);
		finish.reset(owner, point_finish, ShieldScanOperator::SCAN_ALL);
	}

	environment->sourceGrid.scanPoint(point_start.xi(), point_start.yi(), start);
	environment->sourceGrid.scanPoint(point_finish.xi(), point_finish.yi(), finish);

	if(finish.sources().empty() && start.sources().empty())
		return true;

	if(finish.sources().empty() || start.sources().empty()){
		if(intersect){
			if(finish.sources().empty())
				*intersect = crossLineSpheres(point_start, point_finish, start.sources());
			else
				*intersect = crossLineSpheres(point_start, point_finish, finish.sources());
		}
		return false;
	}
			
	    
	//Определяем внутри каких враждебных полей исходная точка (без учета своих, так как на вылет из своих полей нет влияния)
	//Определяем внутри каких враждебных полей конечная точка (без учета своих, так как свои не препятствуют подлету)
	//убираем пересечения, т.к. это означает, что точки внутри одного и того же поля

	sort2(start.sources());
	sort2(finish.sources());

	Shields::iterator it1 = start.sources().begin();
	Shields::iterator it2 = finish.sources().begin();
	
	Shields::iterator end1 = start.sources().end();
	Shields::iterator end2 = finish.sources().end();
	
	while (it1 != end1 && it2 != end2){
		if (*it1 < *it2)
			++it1;
		else if (*it2 < *it1)
			++it2;
		else {
			*it1 = 0;
			*it2 = 0;
			++it1;
			++it2;
		}
	}
	
	start.sources().erase(remove(start.sources().begin(), start.sources().end(), (Shields::value_type)0), start.sources().end());
	finish.sources().erase(remove(finish.sources().begin(), finish.sources().end(), (Shields::value_type)0), finish.sources().end());

	//теперь у нас ест список преград чужих полей из которых нужно вылететь /start.sources()/ 
	//и список чужих полей в которые нужно влететь /finish.sources()/
	
	if(finish.sources().empty() && start.sources().empty())
		return true;

	if(finish.sources().empty() || start.sources().empty()){
		if(intersect){
			if(finish.sources().empty())
				*intersect = crossLineSpheres(point_start, point_finish, start.sources());
			else
				*intersect = crossLineSpheres(point_start, point_finish, finish.sources());
		}
		return false;
	}

	if(intersect){
		Vect3f p1 = crossLineSpheres(point_start, point_finish, start.sources());
		Vect3f p2 = crossLineSpheres(point_start, point_finish, finish.sources());
		if(point_start.distance2(p1) > point_start.distance2(p2))
			*intersect = p2;
		else
			*intersect = p1;
	}
	
	return false;
}

bool SourceShield::traceShieldsThrough(const Vect3f& point_start, const Vect3f& point_finish, const Player* owner, Vect3f* intersect, Vect3f* sphereCenter)
{
	static ShieldScanAllOperator scan;
	scan.reset(owner, owner ? ShieldScanAllOperator::SCAN_ENEMY : ShieldScanAllOperator::SCAN_ALL);

	environment->sourceGrid.Line(point_start.xi(), point_start.yi(), point_finish.xi(), point_finish.yi(), scan);

	if(!scan.sources().empty()){
		Vect3f cros = crossLineSpheres(point_start, point_finish, scan.sources(), sphereCenter);

		if(cros.x > 0.f){
			if(intersect)
				*intersect = cros;
			return false;
		}
	}

	return true;
}

namespace TraceShieldsHelpers{
class ShieldScanDeltaOperator
{
public:
	ShieldScanDeltaOperator(const Vect3f& p1, const Vect3f& p2, const Player* player) 
		: player_(player), p1_(p1), p2_(p2), t_(2), source_(0) {}

	void operator()(SourceBase* src){
		if(src->type() == SOURCE_SHIELD && src->active() && player_->isEnemy(src->player())){
			const SourceShield* shield = safe_cast<const SourceShield*>(src);
			Vect3f p0 = shield->sphereCenter();
			Vect3f p1_p0 = p1_ - p0;
			Vect3f p2_p1 = p2_ - p1_;
			float a = p2_p1.norm2();
			float b = p2_p1.dot(p1_p0);
			float c = p1_p0.norm2() - sqr(shield->sphereRadius());
			float d = sqr(b) - a*c;
			if(d > 0){
				d = sqrtf(d);
				float t1 = (-b + d)/a;
				float t2 = (-b - d)/a;
				if(t2 < t1 && t2 > 0 || t1 < 0)
					t1 = t2;
				if(t_ > t1){
					t_ = t1;
					source_ = shield;
				}
			}
		}
	}

	bool found() const { return t_ < 1; }

	Vect3f intersection() const { Vect3f p; p.interpolate(p1_, p2_, t_); return p; }
	const SourceShield* shield() const { return source_; }

private:
	const Player* player_;
	Vect3f p1_, p2_;
	float t_;
	const SourceShield* source_;
};
}

bool SourceShield::traceShieldsDelta(const Vect3f& start, const Vect3f& finish, const Player* owner, Vect3f& intersection, Vect3f& center)
{
	if(start.distance2(finish) < FLT_EPS)
		return false;
	ShieldScanDeltaOperator op(start, finish, owner);
	environment->sourceGrid.scanPoint(finish.xi(), finish.yi(), op);
	if(op.found()){
		intersection = op.intersection();
		center = op.shield()->sphereCenter();
		return true;
	}
	return false;
}

void SourceShield::shieldExplodeEffect(const Vect3f& center, const Vect3f& pos, const EffectAttribute& effect)
{
	if(effect.isEmpty())
		return;

	Vect3f normal;
	normal.sub(pos, center);
	normal.Normalize();

	BaseUniverseObject ancor;
	ancor.setPose(Se3f(QuatF(acosf(dot(normal, Vect3f::K)), Vect3f::K % normal), pos), true);

	EffectController controller(&ancor);
	controller.effectStart(&effect);

	if(debugShowEnabled)
		controller.showDebugInfo();
	controller.release();
}
