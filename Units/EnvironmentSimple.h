#ifndef __ENVIRONMENT_SIMPLE_H__
#define __ENVIRONMENT_SIMPLE_H__

#include "UnitEnvironment.h"
#include "Water\SpringDamping.h"
#include "Water\FallLeaves.h"

class UnitEnvironmentSimple : public UnitEnvironment
{
public:
	enum TreeType {
		FALL_AND_DISAPPEAR,
		FALL_AND_LIE,
		FALL_AND_LIE_OR_FLOAT,
		FALL_AND_GROW
	};

	enum TreeMode {
		TREE_NORMAL,
		TREE_FALLING,
		TREE_GROWING,
		TREE_FLOATING
	};

	UnitEnvironmentSimple(const UnitTemplate& data);
	~UnitEnvironmentSimple();

	void Quant();
	void fowQuant();
	void graphQuant(float dt);

	void setModel(const char* name);
	
	cSimply3dx* modelSimple() const { return modelSimple_; }
	c3dx* get3dx() const { return modelSimple_; }

	void releaseSpringDamping3DX();

	bool checkInPathTracking(const UnitBase* tracker) const;

	bool checkInBuildingPlacement() const;

	void setPose(const Se3f& poseIn, bool initPose);

	void serialize(Archive& ar);

	void treeQuant();
	void treeRebirth();

	void Kill();

	void explode();
	void collision(UnitBase* p, const ContactInfo& contactInfo);

	void startFall(const Vect3f& point);
	void enableBoxMode();
	void stopFall();

	void setOpacity(float opacity);

	void showDebugInfo();
	void showEditor();

	void mapUpdate(float x0,float y0,float x1,float y1);

	bool setAbnormalState( const AbnormalStateAttribute& state, UnitBase* ownerUnit);

private:
	cSimply3dx* modelSimple_;

	SpringDamping3DX* springDamping3DX_;

	TreeType treeType_;
	bool modelBurnt_;
	bool destructibleFence_;
	float waterWeight_;
	float stoneMass_;

	TreeMode treeMode_;
	LogicTimer fallTimer_;
	LogicTimer fallingLeavesBlowTimer_;
	Se3f initialPose_;
	int growthPeriod_;
	InterpolationLogicTimer growthTimer_;
	bool fallLeavesEnabled_;
	HandleFallLeaves fallLeavesHandle_;
};

#endif //__ENVIRONMENT_SIMPLE_H__
