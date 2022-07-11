#ifndef __LIGHTNING_SOURCE_H_INCLUDED__
#define __LIGHTNING_SOURCE_H_INCLUDED__

#include "SourceEffect.h"
#include "ChainLightningController.h"

class Archive;
class UnitActing;

typedef vector<Vect3f> Vect3fVect;

struct ChainLightningSourceFunctor : public ChainLightningOwnerInterface
{
	ChainLightningSourceFunctor(SourceBase* zone) : zone_(zone) {}
	Vect3f	position()						const	{ return zone_->position();		}
	bool	canApply(UnitActing* unit)		const;
	ParameterSet damage()					const;
	UnitBase* owner()						const	{ return zone_->owner();		}

private:
	SourceBase* zone_;
};

class SourceLightning : public SourceDamage
{
public:
	enum AllocationType {
		TERRA_CENTER,
		TERRA_ROUND,
		TERRA_RANDOM,
		SPHERE_SPHERE,
		SPHERE_RANDOM
	};

	SourceLightning();
	SourceLightning(const SourceLightning& original);
	~SourceLightning();

	SourceBase* clone () const {
		return new SourceLightning(*this);
	}
	SourceType type() const { return SOURCE_LIGHTING; }

	void showEditor() const;
	void showDebug() const;
	
	void serialize(Archive& ar);

	void quant();

	void setRadius(float radius);
	void setPose(const Se3f &_pos, bool init);

	void apply(UnitBase* target);

protected:
	void effectStart();
	void effectStop();

	void start();
	void stop();

	bool killRequest();

private:

	void Update();

	Vect3f setHeight(int x, int y, int h = 0);
	void setTarget(cEffect* eff, const Vect3f& center, const Vect3fVect& ends);
	void turnOff(cEffect*& eff);

	/// постоянная молния в зоне
	EffectReference permanent_effect_;
	cEffect* lightning_;
	/// количество отвилок молнии
	int num_lights_;
	/// высота образования молнии
	float height_;
	/// принцип размещения
	AllocationType alloc_type_;
	Vect3f center_;
	Vect3fVect light_ends_;

	/// выключать молнии в зоне при входе врага
	bool turnOfByTarget_;
	/// параметры отдельной молнии для атаки
	EffectReference  strike_effect_;
	cEffect* strike_;

	Vect3fVect strike_ends_;
	
	UnitActingList targets_;

	bool killInProcess_;

	bool useChainLightning_;
	ChainLightningAttribute chainLightningAttribute_;
	ChainLightningSourceFunctor chainLightningFunctor_;
	ChainLightningController chainLightning_;
};

#endif // #ifndef __LIGHTNING_SOURCE_H_INCLUDED__
