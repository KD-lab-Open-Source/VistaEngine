#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include "Serialization\StringTableReference.h"
#include "Serialization\SerializationTypes.h"
#include "FormulaString.h"
#include "Timers.h"
#include "AttributeReference.h"
#include "ShowChangeController.h"
#include "LocString.h"

class WBuffer;
class UnitInterface;
class ParameterArithmeticsMultiplicator;

/////////////////////////////////////////
class ParameterType : public StringTableBase
{
public:
	enum Type {
		HEALTH,
		HEALTH_RECOVERY,
		ARMOR,
		ARMOR_RECOVERY,
		POSSESSION,
		POSSESSION_RECOVERY,
		POSSESSION_RECOVERY_BACK,
		VELOCITY,
		RESOURCE_PICKING_TIME,
		RESOURCE_PRODUCTIVITY_FACTOR,
		FIRE_RANGE,
		FIRE_RANGE_MIN,
		FIRE_RANGE_EFFECTIVE,
		FIRE_TIME,
		FIRE_DISPERSION,
		RELOAD_TIME,
		RELOAD_TIME_INVENTORY,
		SIGHT_RADIUS,
		NOISE_RADIUS,
		HEARING_RADIUS,
		NUMBER_OF_UNITS,
		CONSTRUCTION_TIME_FACTOR_ON_WATER,
		AMMO,
		AMMO_CAPACITY,
		WEAPON_DURABILITY,
		VARIABLE,
		OTHER,
		OTHER_RECOVERY
	};

	explicit ParameterType(const char* name = "");

	Type type() const { return type_; }
	int counter() const { return counter_; }
	
	const char* comment() const { return comment_.c_str(); }
	void setComment(const char* comment){ comment_ = comment; }

	const wchar_t* tipsName() const { return tipsName_.c_str(); }
	const char* label() const { return tipsName_.key(); }

	void serialize(Archive& ar);
	void setCounter();

private:
	string comment_;
	LocString tipsName_;
	Type type_;
	int counter_;
};

typedef StringTable<ParameterType> ParameterTypeTable;
typedef StringTableReference<ParameterType, true> ParameterTypeReference;
typedef StringTableReference<ParameterType, false> ParameterTypeReferenceZero;

class ParameterFormula : public StringTableBase
{
public:
    ParameterFormula (const char* name = "Formula")
      : StringTableBase(name)
    {
    }
    const FormulaString& formula () const { return formula_; }
    void setFormula (const FormulaString& formula) {
        formula_ = formula;
    }
	void serialize (Archive& ar);
private:
    FormulaString formula_;
};

typedef StringTable<ParameterFormula> ParameterFormulaTable;
typedef StringTableReference<ParameterFormula, true> ParameterFormulaReference;

struct ParameterGroup : StringTableBaseSimple
{
	ParameterGroup(const char* name = "") : StringTableBaseSimple(name) {}
};

typedef StringTable<ParameterGroup> ParameterGroupTable;
typedef StringTableReference<ParameterGroup, true> ParameterGroupReference;

class ParameterFormulaString : public FormulaString{
public:
	static float parameterValue_;
	static ParameterGroupReference parameterGroup_;

	ParameterGroupReference group_;
	float value_;

	void serialize(Archive& ar);
};

class ParameterValue : public StringTableBase
{
public:
	enum ParameterState {
		NOT_CALCULATED,
		CALCULATED,
		CALCULATING
	};

	ParameterValue(const char* name = "")
	: StringTableBase(name)
	, value_(0)
	, state_(NOT_CALCULATED)
	, calculated_value_(0)
	, can_modify_(true)
	{}

	float value() const;

	// неподсчитанное значение:
	float rawValue() const;
	void setRawValue(float value) 
	{ 
		value_ = value; 
		state_ = NOT_CALCULATED;
	}

	bool canModify() const { return can_modify_; }

	const ParameterGroupReference& group() const { return group_; }
	void setGroup(ParameterGroupReference _group) { group_ = _group; }
	const ParameterTypeReference& type() const { return type_; }
	void setType(ParameterTypeReference _type) { type_ = _type; }
	
    const ParameterFormulaReference& formula() const { return formula_; }
	void setFormula(ParameterFormulaReference _formula) { formula_ = _formula; }
	void serialize(Archive& ar);

	// дл€ редактора:
	void			editorGroupMoveBefore(int index, int beforeIndex);
	string			editorGroupName() const{ return group_.c_str(); }
	static void     editorAddGroup(const char* name);
	static const char* editorGroupsComboList();
	static bool     editorDynamicGroups() { return true; }
	void            editorSetGroup(const char* group){ group_ = ParameterGroupReference(group); }
	bool		    editorVisible() const{ return true; }

private:
	float value_;

	mutable float calculated_value_;
	mutable ParameterState state_;

	ParameterGroupReference group_;
	ParameterTypeReference type_;
    ParameterFormulaReference formula_;

	bool can_modify_;
};

typedef StringTable<ParameterValue> ParameterValueTable;
typedef StringTableReference<ParameterValue, true> ParameterValueReference;

bool matchMask(const char* mask, const char* text);

struct LookupParameter
{
	ParameterGroupReference group_;

	LookupParameter(const ParameterGroupReference& group)
	: group_(group)
	{}

	bool operator()(const char* name, float& value);
};

////////////////////////////////////////////////////////////////////
// 1. ѕредполагаетс€, что все исходные значени€ и результаты >= 0, 
// поэтому при вычитании значени€ ограничиваютс€ снизу.
// 2. “еперь хран€тс€ не все значени€, большинство операций работает на 
// пересечении множеств.
// 3. јккуратно пользоватьс€ конструкторами: пустой создает пустое множество,
// с одним параметром - все типы, копировани€ - только те, что были.
class ParameterSet 
{
public:
	struct Value {
		float value;
		int index; // индекс в ParameterTypeTable

		Value(){}
		Value(float val, int idx) : value(val), index(idx) {}
		void serialize(Archive& ar);
	};

	ParameterSet(); // Ќе создает значений, даже нулевых

	const ParameterSet& operator=(const ParameterSet& set) {
		values_ = set.values_;
		return *this;
	}

	void set(float value);
	void set(float value, ParameterType::Type type); 
	void set(const ParameterSet& set);
	void addByIndex(float value, int index);
	void mask(ParameterType::Type type); // «анул€ет все, кроме type
	void mask(const ParameterSet& set); // «анул€ет все, кроме типов в set
	
	ParameterSet& operator+=(const ParameterSet& rhs);
	ParameterSet& operator*=(float k);

	void sub(const ParameterSet& rhs); // ћожет быть ниже нул€
	bool subClamped(const ParameterSet& rhs); // ќбрезает все, что ниже 0,  true при обрезании
	void scaleAdd(const ParameterSet& parameters, float k);

	float dot(const ParameterSet& rhs) const;

	void clamp(const ParameterSet& upperBound);
	void setArmor(const ParameterSet& currentSet, const ParameterSet& maxSet); // ¬ыставл€ет maxSet[ARMOR], где currentSet[ARMOR] != 0 в *this[HEALTH]
	void subPositiveOnly(const ParameterSet& rhs); // Ќе вычитает броню из отрицательных значений
	void setRecovery(const ParameterSet& parameters); // ¬ыставл€ет [HEALTH_RECOVERY], [ARMOR_RECOVERY], [OTHER_RECOVERY] в [HEALTH], [ARMOR], [OTHER] соответственно
	void setPossessionRecovery(const ParameterSet& parameters, const ParameterSet& parametersMax); // ¬ыставл€ет this[POSESSION] = parameters[POSESSION_RECOVERY]
	void setPossessionRecoveryBack(const ParameterSet& parameters); // ¬ыставл€ет this[POSESSION] = parameters[POSESSION_RECOVERY_BACK]
	bool restorePossessionRecoveryBack(const ParameterSet& parametersMax); // ¬ыставл€ет this[POSSESSION_RECOVERY_BACK] = parametersMax[POSSESSION_RECOVERY_BACK] по минимальному POSSESSION
	void updateRange(ParameterSet& prm_min, ParameterSet& prm_max) const; // ¬ычисл€ет диапазон [HEALTH], [ARMOR], расшир€ет его в min, max если надо

	float sum() const;
	float health() const; // Ќаходит наименьшее здоровье
	float armor() const; // Ќаходит наименьшую броню
	float possession() const; // Ќаходит наименьшее владение
	float maxByType(ParameterType::Type type) const;

	float minFraction(const ParameterSet& parametersMax) const;
	void scaleByProgress(float factor); // ”множает на factor здоровье, броню и владение, которые растут при строительстве
	void scaleType(ParameterType::Type type, float factor);

	bool contain(const ParameterTypeReference& type) const; 
	bool contain(const ParameterSet& subSet) const; // все типы из subSet содержатьс€ в this
	bool containAny(const ParameterSet& subSet) const; // хот€ бы один тип из subSet содержитс€ в this
	bool above(const ParameterSet& level) const; // this >= any values of level
	float progress(const ParameterSet& resource1, const ParameterSet& resource2) const; // минимальный из clamp(max{ resource1, resource2 }  / this, 0, 1)
	bool below(const ParameterSet& resource1, const ParameterSet& resource2) const; // this < any value { resource1, resource2 }
	bool empty() const { return values_.empty(); }
	bool zero() const; // all zero
	bool zero(const ParameterSet& filter) const; // все, где filter > 0, == 0

	float findByType(ParameterType::Type type, float defaultValue) const;
	float findByIndex(int index, float defaultValue = 0.f) const; // Ћинейный поиск, индекс в ParameterTypeTable
	float findByLabel(const char* label) const;
	float findByName(const char* name, float defaultValue) const;

	void toString(WBuffer& out) const;
	void printSufficient(WBuffer& out, const ParameterSet& resource, const wchar_t* enableColor, const wchar_t* disableColor, const char* paramLabel = 0) const;
	
	int size() const { return values_.size(); }
	const Value& operator[](int index) const { return values_[index]; }

	void serialize(Archive& ar);
	
	void read(XBuffer& buffer);
	void write(XBuffer& buffer) const;

	void showDebug(const Vect3f& position, const Color4c& color) const;
	const char* debugStr() const;

protected:
	typedef vector<Value> Values;
	Values values_;

	static const float eps;

	friend class ArithmeticsData;
};

class ParameterCustom : public ParameterSet // »спользовать только в местах редактировани€, дл€ временных переменных - ParameterSet
{
public:
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	void add(const ParameterCustom& parameters); // for conversion only

	typedef vector<ParameterValueReference> Vector;

	const Vector& customVector() const{ return vector_; }
private:
	Vector vector_;
	
	friend struct ParameterShowSetting;
};

class ArithmeticsData
{
public:
	enum CreationType
	{
		OLD = 1,
		NEW = 2,
		ALL = 3
	};

	enum UnitType 
	{
		TAKEN = 1,
		TAKEN_TYPE = 2,
		CHOSEN_TYPES = 4,
		ALL_TYPES = 8,
		PLAYER = 16,
		PLAYER_CAPACITY = 32
	};

	enum WeaponType 
	{
		WEAPON_NONE,
		WEAPON_ANY,
		WEAPON_SHORT_RANGE,
		WEAPON_LONG_RANGE,
		WEAPON_TYPES
	};

	enum Address
	{
		UNIT = 1,
		UNIT_MAX = 2,
		
		WEAPON = 4,
		SOURCE = 8,
		DAMAGE = 16,
		
		WATER_DAMAGE = 32,
		LAVA_DAMAGE = 64,
		ICE_DAMAGE = 128,
		EARTH_DAMAGE = 256
	};

	enum Operation 
	{
		SET,
		ADD,
		ADD_PERCENT,
		SUB,
		SUB_PERCENT,
	};

	BitVector<UnitType> unitType;
	CreationType creationType;
	WeaponType weaponType;
	bool influenceInStatistic_;
	BitVector<Address> address;
	AttributeUnitOrBuildingReference attribute;
	WeaponPrmReference weapon;

	ParameterTypeReference parameterType;
	Operation operation;
	float value;

	ArithmeticsData();
	void serialize(Archive& ar);
	void apply(ParameterSet& parameters) const;
	void apply(ParameterSet& parameters, const ParameterSet& parametersMax) const;
	void reserve(ParameterSet& parameters) const; // зарезервировать нулевые значени€, иначе apply ничего не добавит
	void setInverted() { inverted_ = true; }
	void operator*=(float k);
	bool checkWeapon(const WeaponPrm* weaponPrm) const;

	void getUIData(WBuffer& buf) const;

private:
	bool inverted_;
	
	void apply(float& lvalue) const;
};


class ParameterArithmetics
{
public:
	ParameterArithmetics() {}
	ParameterArithmetics(const ParameterArithmetics& origin, bool inverted);

	void serialize(Archive& ar);
	void operator*=(float k);
	void operator*=(const ParameterArithmeticsMultiplicator& multi);

	void reserve(ParameterSet& parameters) const;
	void apply(ParameterSet& parameters) const;

	typedef vector<ArithmeticsData> Data;
	Data data;

	bool getUIData(WBuffer& buf, const char* name) const;
	void getUIData(WBuffer& buf) const;
};


class ParameterArithmeticsMultiplicator 
{
public:
	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:
	struct Value {
		ParameterTypeReference type;
		float factor;
		Value();
		void serialize(Archive& ar);
	};

	typedef	vector<Value> Values;
	Values values_;

	friend ParameterArithmetics;
};

class ShowChangeParametersController
{
	typedef SwapVector<pair<int, SharedShowChangeController> > ShowChangeControllers;
	ShowChangeControllers controllers_;

public:
	enum CreateType {
		SET, // вз€ть только множество дл€ накоплени€ и забить значени€ нул€ми
		VALUES, // вз€ть множество со значени€ми параметров
		SHOW // показать текущие значени€ параметров
	};
	void create(const UnitInterface* owner, const ParameterSet& showDelta, CreateType createType);

	ShowChangeParametersController() {}
	ShowChangeParametersController(const UnitInterface* owner, const ParameterSet& showDelta, CreateType createType = SHOW) { create(owner, showDelta, createType); }
	~ShowChangeParametersController() { clear(); }

	void setOwner(const UnitInterface* owner);
	void clear();

	void add(const ParameterSet& addVal);
	void update(const ParameterSet& newVal);
};


class ParameterConsumer // на смену EnergyConsumer
{
public:
	ParameterConsumer();
	void clear();
	void start(const UnitInterface* showOwner, const ParameterSet& total, float time);
	bool addQuant(const ParameterSet& resource, float factor = 1.f); // true when ready
	float progress() const;
	bool started() const { return progress() > FLT_EPS; }
	void setProgress(float progress);
	const ParameterSet& delta() const { return delta_; }

	void serialize(Archive& ar);

private:
	float totalSum_;
	ParameterSet current_;
	ParameterSet delta_;
	float timer_;
	float timerMax_;
	ShowChangeParametersController showControllers_;
};

enum ShowEvent {
	SHOW_AT_NOT_HOVER_OR_SELECT,
	SHOW_AT_SELECT,
	SHOW_AT_HOVER,
	SHOW_AT_HOVER_OR_SELECT,
	SHOW_AT_PARAMETER_CHANGE,
	SHOW_AT_PARAMETER_INCREASE,
	SHOW_AT_PARAMETER_DECREASE,
	SHOW_AT_BUILD,
	SHOW_AT_BUILD_OR_UPGRADE,
	SHOW_AT_DIRECT_CONTROL,
	SHOW_AT_PRODUCTION,
	SHOW_ALWAYS
};

struct ParameterShowSetting
{
	enum ParameterClass {
        LOGIC,
		PRODUCTION_PROGRESS,
		UPGRADE_PROGRESS,
		FINISH_UPGRADE, // стади€ окончани€ апгрейда
		GROUND_PROGRESS,
		RELOAD_PROGRESS
	};
	enum AlignType {
		ALIGN_LEFT,
		ALIGN_CENTER
	};
	enum Shape {
		BAR,
		CIRCLE
	};

	ParameterClass type_;
	
	ParameterTypeReference paramTypeRef;
	WeaponPrmReference weaponPrmRef;
	int dataType;

	Vect2f paramOffset;
	AlignType alignType;
	ShowEvent showEvent;
	bool notShowEmpty;

	Color4c minColor;
	Color4c maxColor;

	float radius;
	int delayShowTime;

	Shape shape;

	/// ѕолоска
	Color4f borderColor;
	Color4f backgroundColor;

	///  руг
	float innerRadius;
	float startAngle;
	float endAngle;
	bool direction;
	
	ShowChangeSettings showChangeSettings;
	
	ParameterShowSetting();
	void serialize(Archive& ar);

	static ParameterCustom* possibleParameters_;
};

struct ShowChangeParameterSetting{
	ShowChangeParameterSetting() {}
	void serialize(Archive& ar);

	int typeIdx() const { return typeRef_.key(); }
	const ShowChangeSettings& changeSettings() const { return changeSettings_; }

private:
	ParameterTypeReference typeRef_;
	ShowChangeSettings changeSettings_;
};

typedef vector<ShowChangeParameterSetting> ShowChangeParameterSettings;

// параметры наносимых юнитом повреждений
struct WeaponDamage : ParameterCustom
{
	WeaponDamage() {
	}

	void serialize(Archive& ar);
};

#endif //__PARAMETERS_H__
