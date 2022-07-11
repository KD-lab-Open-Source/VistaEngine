#ifndef __CHAIN_LIGHTNING_CONTROLLER_H_INCLUDED__
#define __CHAIN_LIGHTNING_CONTROLLER_H_INCLUDED__

#include "..\Units\UnitLink.h"
#include "Timers.h"
#include <set>

#include "EffectReference.h"
#include "..\Units\AbnormalStateAttribute.h"

typedef set<int> UnitCache;

class UnitBase; 
class UnitActing;

class Archive;

typedef vector<UnitActing*> UnitActingList;

struct ChainLightningOwnerInterface
{
	virtual Vect3f	position() const = 0;
	virtual bool	canApply(UnitActing*) const = 0;
	virtual ParameterSet damage() const  = 0;
	virtual UnitBase* owner() const = 0;
};


struct ChainLightningAttribute{
	/// сколько раз молния может переотразится
	int chainFactor_;
	/// сколько отвилок может быть от одного юнита 
	int unitChainFactor_;

	/// в каком радиусе действует цепной эффект
	float chainRadius_;
	/// максимальная длина отраженной молнии
	float unitChainRadius_;

	/// параметры отдельной молнии для атаки
	EffectReference  strike_effect_;

	/// время распространения молнии на все доступные уровни
	float spreadingTime_;

	/// воздействие на юниты
	AbnormalStateAttribute abnormalState_;

	ChainLightningAttribute();
	void serialize(Archive& ar);
};

class ChainLightningController
{
public:
	ChainLightningController(ChainLightningOwnerInterface* owner = 0);
	~ChainLightningController();

	// update можно на каждом кванте не вызывать
	bool needChainUpdate() const { return !grafValid_ || chainUpdateTimer_(); }
	// перестроить дерево молний
	void update(const UnitActingList& lightningsEmitters);

	void start(const ChainLightningAttribute* attr);
	void stop();
	bool active() const { return phase_ != NOT_STARTED; }

	// отрисовка и нанесение урона
	void quant();

	// немедленная остановка
	void release();

	void showDebug() const;

private:
	const ChainLightningAttribute* attr_;
	ChainLightningOwnerInterface* owner_;

	/// повреждения, наносимые за квант
	ParameterSet damage_;

	bool grafValid_;
	DelayTimer chainUpdateTimer_;
	
	enum Phase {
		NOT_STARTED,
		FADE_ON,
		WORKING,
		FADE_OFF
	};
	Phase phase_;
	MeasurementTimer phaseTime_;
	bool checkLevel(int level) const;

	struct ChainNode;
	typedef list<ChainNode> ChainNodes;

	typedef UnitLink<UnitActing> UnitActingLink;

	struct ChainNode{
		UnitActingLink unitNode;
		int strikeIndex;
		int size;
		ChainNodes graf;

		ChainNode(UnitActing* unit)
			: unitNode(unit)
			, strikeIndex(-1)
			, size(0) {}

			Vect3f center() const;
	};
	typedef vector<ChainNode*> ChainNodesLinks;

	ChainNodes chainGraf_;
	UnitCache unitCache_;

	typedef vector<cEffect*> Effects;
	Effects effects;

	void turnOff(cEffect*& eff);

	bool chainStrikeRecurse(ChainNode& node, int deep, bool release = false);
	void showDebugChainRecurse(const ChainNode& node, int deep) const;

	void apply(UnitActing* target, int levelFactor);
};

#endif // #ifndef __CHAIN_LIGHTNING_CONTROLLER_H_INCLUDED__
