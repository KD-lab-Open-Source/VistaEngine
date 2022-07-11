#ifndef __ATTRIBUTE_REFERENCE_H__
#define __ATTRIBUTE_REFERENCE_H__

#include "Serialization\StringTableBase.h"
#include "Serialization\StringTableReference.h"
#include "Serialization\StringTableReferencePolymorphic.h"

struct RaceProperty;
typedef StringTable<RaceProperty> RaceTable;
typedef StringTableReference<RaceProperty, true>  Race;

struct DifficultyPrm;
typedef StringTable<DifficultyPrm> DifficultyTable;
typedef StringTableReference<DifficultyPrm, true> Difficulty;

struct UnitNameString : StringTableBaseSimple
{
	UnitNameString(const char* name = "") : StringTableBaseSimple(name) {}
};

typedef StringTable<UnitNameString> UnitNameTable;
typedef StringTableReference<UnitNameString, true> UnitName;

/////////////////////////////////////////
class UnitAttributeID
{
public:
	UnitAttributeID();
	explicit UnitAttributeID(const char* str);
	UnitAttributeID(const UnitName& unitName, const Race& race);

	const Race& race() const { return race_; }
	const UnitName& unitName() const { return unitName_; }

	string nameRace() const;

	bool operator<(const UnitAttributeID& rhs) const {
		return race_ == rhs.race_ ? unitName_ < rhs.unitName_ : race_ < rhs.race_;
	}

	bool operator==(const UnitAttributeID& rhs) const {
		return unitName_ == rhs.unitName_ && race_ == rhs.race_ ;
	}

	void serialize(Archive& ar);

private:
	UnitName unitName_;
	Race race_;
};

class AttributeBase;

class UnitAttribute : public StringTableBasePolymorphic<AttributeBase>
{
	typedef StringTableBasePolymorphic<AttributeBase> BaseClass;
public:
	UnitAttribute(const char* name = "");
	UnitAttribute(UnitAttributeID unitAttributeID, AttributeBase* attribute = 0);

	void serialize(Archive& ar);

	const UnitAttributeID& key() const { return unitAttributeID_; }
	void setKey(const UnitAttributeID& key);
	void setName(const char* name);

	// editor stuff
	const char*	editorName() const;
	static bool editorAllowRename();
	void editorCreate(const char* name, const char* groupName);
	string editorGroupName() const;
	static const char* editorGroupsComboList();

private:
	UnitAttributeID unitAttributeID_;
};

typedef StringTable<UnitAttribute> AttributeLibrary;

class AttributeReference : public StringTableReferencePolymorphic<AttributeBase, false, UnitAttribute>
{
	typedef StringTableReferencePolymorphic<AttributeBase, false, UnitAttribute> BaseClass;

public:
	AttributeReference() {}
	AttributeReference(const AttributeBase* attribute);
	AttributeReference(const char* attributeName);

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

	UnitAttributeID unitAttributeID() const;

	bool refineComboList() const { return true; }
	const char* editorTypeName() const { return typeid(AttributeReference).name(); }
};

typedef vector<AttributeReference> AttributeReferences;

struct AttributeUnitReference : AttributeReference
{
	AttributeUnitReference() {}
	AttributeUnitReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeUnitReference).name(); }
};
typedef vector<AttributeUnitReference> AttributeUnitReferences;

struct AttributeBuildingReference : AttributeReference
{
	AttributeBuildingReference() {}
	AttributeBuildingReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeBuildingReference).name(); }
};
typedef vector<AttributeBuildingReference> AttributeBuildingReferences;

struct AttributeUnitOrBuildingReference : AttributeReference
{
	AttributeUnitOrBuildingReference() {}
	AttributeUnitOrBuildingReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeUnitOrBuildingReference).name(); }
};
typedef vector<AttributeUnitOrBuildingReference> AttributeUnitOrBuildingReferences;

struct AttributeUnitActingReference : AttributeReference
{
	AttributeUnitActingReference() {}
	AttributeUnitActingReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeUnitActingReference).name(); }
};
typedef vector<AttributeUnitActingReference> AttributeUnitActingReferences;

struct AttributeItemReference : AttributeReference
{
	AttributeItemReference() {}
	AttributeItemReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeItemReference).name(); }
};
typedef vector<AttributeItemReference> AttributeItemReferences;

struct AttributeItemResourceReference : AttributeReference
{
	AttributeItemResourceReference() {}
	AttributeItemResourceReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeItemResourceReference).name(); }
};
typedef vector<AttributeItemResourceReference> AttributeItemResourceReferences;

struct AttributeItemInventoryReference : AttributeReference
{
	AttributeItemInventoryReference() {}
	AttributeItemInventoryReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeItemInventoryReference).name(); }
};
typedef vector<AttributeItemInventoryReference> AttributeItemInventoryReferences;

struct AttributeUnitObjectiveReference : AttributeReference
{
	AttributeUnitObjectiveReference() {}
	AttributeUnitObjectiveReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributeUnitObjectiveReference).name(); }
};
typedef vector<AttributeUnitObjectiveReference> AttributeUnitObjectiveReferences;

struct AttributePadReference : AttributeReference
{
	AttributePadReference() {}
	AttributePadReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributePadReference).name(); }
};

struct AttributePlayerReference : AttributeReference
{
	AttributePlayerReference() {}
	AttributePlayerReference(const AttributeReference& data) : AttributeReference(data) {}

	bool validForComboList(const AttributeBase& attr) const;
	const char* editorTypeName() const { return typeid(AttributePlayerReference).name(); }
};

/////////////////////////////////////////
enum AuxAttributeID 
{
	AUX_ATTRIBUTE_NONE,
	AUX_ATTRIBUTE_ENVIRONMENT,
	AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE,
	AUX_ATTRIBUTE_DETONATOR,
	AUX_ATTRIBUTE_ZONE,
	AUX_ATTRIBUTE_PLAYER_UNIT
};

class AuxAttribute : public StringTableBasePolymorphic<AttributeBase>
{
	typedef StringTableBasePolymorphic<AttributeBase> BaseClass;
public:
	AuxAttribute(const char* name = "");
	AuxAttribute(AuxAttributeID auxAttributeID);

	void serialize(Archive& ar);
};

typedef StringTable<AuxAttribute> AuxAttributeLibrary;

class AuxAttributeReference : public StringTableReferencePolymorphic<AttributeBase, false, AuxAttribute>
{
	typedef StringTableReferencePolymorphic<AttributeBase, false, AuxAttribute> BaseClass;
public:
	AuxAttributeReference() {}
	AuxAttributeReference(const AttributeBase* attribute);
	AuxAttributeReference(AuxAttributeID auxAttributeID);

	const char* editorTypeName() const { return typeid(AuxAttributeReference).name(); }
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

class WeaponPrm;
typedef StringTable<StringTableBasePolymorphic<WeaponPrm> > WeaponPrmLibrary;
typedef StringTableReferencePolymorphic<WeaponPrm, false> WeaponPrmReference;
typedef vector<WeaponPrmReference> WeaponPrmReferences;

class UnitFormationType;
typedef StringTable<UnitFormationType> UnitFormationTypes;
typedef StringTableReference<UnitFormationType, true> UnitFormationTypeReference;
typedef vector<UnitFormationTypeReference> UnitFormationTypeReferences;

#endif // __ATTRIBUTE_REFERENCE_H__
