#ifndef __MICRO_AI_H__
#define __MICRO_AI_H__

class UnitActing;
class UnitInterface;

float microAI_TargetPriority(const UnitActing* unit, const UnitInterface* target);

class MicroAI_Scaner
{
public:
	MicroAI_Scaner(UnitActing* owner, AutoAttackMode mode, int weapon_id = 0): owner_(owner),
		currentGroup_(0),
		mode_(mode)
	{
		weaponID_ = weapon_id;

		targets_.reserve(64);
		excludeTargets_.reserve(64);
		enemies_.reserve(64);
		allies_.reserve(64);

		notificationRadius_ = sqr(owner->sightRadius() * owner->attr().attackTargetNotificationRadius);
	}

	void operator()(UnitBase* unit);

	/// Раскидывание целей по оружию.
	/// Возвращает лучщую цель, которая используется для выставления way point'ов.
	UnitInterface* processTargets();

private:

	UnitActing* owner_;
	AutoAttackMode mode_;

	int weaponID_;

	const WeaponGroupType* currentGroup_;

	struct TargetData {
		float priority_;
		float distance_;
		UnitInterface* target_;

		TargetData(UnitInterface* target, float priority = 0.f) : target_(target) {
			priority_ = priority;
			distance_ = 0.f;
		}

		bool operator < (const TargetData& rh) const { return priority_ < rh.priority_; }
		bool operator == (const UnitInterface* unit) const { return target_ == unit; }
	};

	typedef vector<TargetData> Targets;
	Targets targets_;

	float notificationRadius_;

	typedef vector<const UnitInterface*> ExcludeTargets;
	ExcludeTargets excludeTargets_;

	typedef vector<UnitInterface*> Enemies;
	Enemies enemies_;

	typedef vector<UnitActing*> Allies;
	Allies allies_;

	void excludeTarget(const UnitInterface* target)
	{
		excludeTargets_.push_back(target);
	}

	bool isTargetExcluded(const UnitInterface* target) const
	{
		ExcludeTargets::const_iterator it = std::find(excludeTargets_.begin(), excludeTargets_.end(), target);
		return it != excludeTargets_.end();
	}

	bool isTargetFree(const UnitInterface* target) const;

	void addTarget(UnitInterface* target);
	void updateTarget(TargetData& target);
	float targetPriority(const TargetData& target, const WeaponBase* weapon) const;
	bool isVisible(UnitBase* unit, bool trace_visibility = true) const;
	bool checkTransport(UnitBase* unit) const;
	bool canAttack(UnitInterface* unit) const;

	UnitInterface* assignTargets(bool add_targets_to_list);
	/// возвращает наилучшую цель для назначенного через weaponID_ оружия
	UnitInterface* selectedWeaponTarget();
};

class MicroAI_NoiseScaner
{
public:
	MicroAI_NoiseScaner(UnitActing* owner);

	void operator()(UnitBase* unit);

	UnitBase* target() const { return target_; }

private:

	UnitActing* owner_;

	UnitBase* target_;
	float targetDist2_;

	float sightRadius2_;
};

#endif // __MICRO_AI_H__
