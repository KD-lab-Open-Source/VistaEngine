#ifndef __UNIT_OBJECTIVE_H__
#define __UNIT_OBJECTIVE_H__

#include "RealUnit.h"

//////////////////////////////////////////
// Юнит-цель: легионер, здание и предмет
//////////////////////////////////////////
class UnitObjective : public UnitReal
{
public:
	UnitObjective(const UnitTemplate& data);
	~UnitObjective();

	void serialize(Archive& ar);

	void Kill();

	void Quant();
	void fowQuant();

	void graphQuant(float dt);

	bool checkShowEvent(ShowEvent event);
	bool getParameterValue(const ParameterShowSetting& par, float &phase);

	void changeUnitOwner(Player* playerIn);

	bool selectAble() const { return true; }
	const UI_MinimapSymbol* minimapSymbol(bool permanent) const;

	void setPose(const Se3f& poseIn, bool initPose);

	void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics);
	void applyParameterArithmetics(const ParameterArithmetics& arithmetics);

	void setRegisteredInRealUnits(bool registeredInRealUnits) { isRegisteredInRealUnits_ = registeredInRealUnits; }
	bool isRegisteredInRealUnits() { return isRegisteredInRealUnits_; }

	bool isRegisteredInPlayerStatistics() { return isRegisteredInPlayerStatistics_; }
	void setRegisteredInPlayerStatistics(bool registeredInPlayerStatistics) { isRegisteredInPlayerStatistics_ = registeredInPlayerStatistics; }
	void registerInPlayerStatistics();
	void unregisterInPlayerStatistics(UnitBase* agressor = 0);
	
private:
	// Параметры до последних изменений parameters_ (запоминается в начале Quant())
	ParameterSet preParameters_;

	// Время принудительного показа параметра
	//typedef SwapVector<ParameterTimer> ParameterShowTimerContainer;
	typedef SwapVector<DurationTimer> ParameterShowTimerContainer;
	ParameterShowTimerContainer parameterShowTimers_;

	typedef SwapVector<ShowChangeController> ShowChangeControllers;
	ShowChangeControllers showChangeControllers_;

	bool isRegisteredInRealUnits_;
	bool isRegisteredInPlayerStatistics_;

	FOW_HANDLE fow_handle;
};

class ItemHideScaner {
	bool needHide_;
	PlacementZone placementZone_;
public:
	ItemHideScaner(PlacementZone placementZone):needHide_(false), placementZone_(placementZone) {}
	bool needHide() { return needHide_; }
	void operator()(UnitBase* unit);
};

#endif //__UNIT_OBJECTIVE_H__
