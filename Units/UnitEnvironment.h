#ifndef __UNIT_ENVIRONMENT_H__
#define __UNIT_ENVIRONMENT_H__


#include "BaseUnit.h"
#include "UnitAttribute.h"

class UnitEnvironment : public UnitBase
{
public:
	UnitEnvironment(const UnitTemplate& data);

	EnvironmentType environmentType () const { return environmentType_; }
	void setEnvirontmentType(EnvironmentType type) { environmentType_ = type; }

    virtual void setModel(const char* name) = 0;
	const char* modelName() const { return modelName_.c_str(); }

	const HarmAttribute& harmAttr() const { return deathParameters_; }

	void setRadius(float new_radius);
	void setPose(const Se3f& poseIn, bool initPose);

	void Kill();
	void Quant();

	bool checkInPathTracking(const UnitBase* tracker) const;
	virtual bool checkInBuildingPlacement() const;
	void mapUpdate(float x0,float y0,float x1,float y1);

	void serialize(Archive& ar);
    
	void showEditor();
	void showDebugInfo();
	bool setAbnormalState(const AbnormalStateAttribute& state, UnitBase* ownerUnit);
	bool hasAbnormalState(const AbnormalStateAttribute& state) const;

	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_ENVIRONMENT; }

	bool canBeTransparent() const { return canBeTransparent_; }
	bool isInFieldOfViewMap() const { return fieldOfViewMapAdd_; }

protected:
	string modelName_;
	EnvironmentType environmentType_;
	enum ObjectLodPredefinedType lodDistance_; 
	bool ptBoundCheck_;
	bool checkGround_;
	bool checkGroundPoint_;
	bool holdOrientation_;
	bool verticalOrientation_;
	bool lighted_;
	bool burnt_;
	bool destroyInWater_;
	bool destroyInAbnormalState_;
	float scale_;

	HarmAttribute deathParameters_;
	bool hideByDistance;
	bool canBeTransparent_;
	bool fieldOfViewMapAdd_;
	bool fieldOfViewMapAdded_;
};

#endif //__UNIT_ENVIRONMENT_H__
