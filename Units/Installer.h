#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#include "AttributeReference.h"
#include "Timers.h"

class Player;
class AttributeBuilding;
class UnitInterface;
class cObject3dx;

class BuildingInstaller
{
public:
	BuildingInstaller();
	~BuildingInstaller();

	void Clear();
	void InitObject(const AttributeBuilding* attr, bool createHologram);
	void ConstructObject(Player* player, UnitInterface* builder); 

	void SetBuildPosition(const Vect2f& position, float angle);
	void quant(Player* player, Camera* camera);

	void environmentAnalysis(Player* player);

	const AttributeBuilding* attribute() const { return attribute_; }
	const Vect3f& position() const { return position_; }
	float angle() const { return angle_; }
	float angleDiscrete() const;

	bool inited() const { return ObjectPoint != 0; }
	bool valid() const { return valid_; }
	bool buildingInArea() const { return buildingInArea_; }

private:
	const AttributeBuilding* attribute_;
	Vect3f position_;
	Vect2f requestPosition_;
	Vect2f snapPosition_;
	float angle_;
	bool valid_;
	bool buildingInArea_;
	bool multipleInstalling_;
	cObject3dx* ObjectPoint;
	
	char* BaseBuff;
	int BaseBuffSX,BaseBuffSY;
	int OffsetX,OffsetY;

	class cTexture* pTexture;
	class cPlane* plane;

	void InitTexture();
};

#endif _INSTALLER_H_
