#ifndef __UNIT_ITEM_RESOURCE_H__
#define __UNIT_ITEM_RESOURCE_H__

#include "UnitObjective.h"

class AttributeItemResource : public AttributeBase
{
public:
	// элементы неживой природы
	enum ItemType {
		ITEM_DEFAULT,
		ITEM_TREE,
		ITEM_FENCE, 
		ITEM_BUSH,
		ITEM_STONE
	} itemType;

	int appearanceDelay;
	bool useLifeTime;
	int lifeTime;

	/// прятать при установки здания
	bool enableHiding;

	bool barrierUpgrade;
    
	AttributeItemResource();
	void serialize(Archive& ar);
	bool isChainNecessary(ChainID chainID) const;
};


class UnitItemResource : public UnitObjective
{
public:
	const AttributeItemResource& attr() const { return safe_cast_ref<const AttributeItemResource&>(UnitReal::attr()); }

	UnitItemResource(const UnitTemplate& data);

	void Quant();
	void executeCommand(const UnitCommand& command) {}
	void collision(UnitBase* p, const ContactInfo& contactInfo);
	bool isUnseen() const { return (hiddenLogic() & (HIDE_BY_PLACED_BUILDING | HIDE_BY_INITIAL_APPEARANCE)) != 0; }
	float parametersSum() const { return parametersSum_; }

private:
	void hideCheck();

	float parametersSum_;
	LogicTimer appearanceTimer_;
	LogicTimer killTimer_;
};

#endif //__UNIT_ITEM_H__
