#ifndef __NATURE_H__
#define __NATURE_H__

#include "UnitEnvironment.h"

class UnitEnvironmentBuilding : public UnitEnvironment
{
public:
	UnitEnvironmentBuilding(const UnitTemplate& data);
	~UnitEnvironmentBuilding();

	void Quant();
	void dayQuant(bool invisible);
	void fowQuant();

	void setPose(const Se3f& pose, bool initPose);

	void setModel(const char* name);
	const char* modelName() const { return modelName_.c_str(); }

	void serialize(Archive& ar);

	void explode();
	void Kill();

	cObject3dx* model() const { return model_; }
	c3dx* get3dx() const { return model_; }

	void collision(UnitBase* p, const ContactInfo& contactInfo);

	void setChain(const AnimationChain* chain); 
	const AnimationChain* getChain(int animationGroup = 0) const; 

	void mapUpdate(float x0,float y0,float x1,float y1);

	void showEditor();
	
	const UnitColor& defaultColor() const { return permanentColor_; }

private:
	cObject3dx* model_;
	UnitColor permanentColor_;
	ChainControllers chainControllers_;

	AnimationChain animationChain_;
	AnimationChain animationChainBurnt_;
	float deviationCosMin_;
};

#endif //__NATURE_H__
