#ifndef __UNIT_ID_H__
#define __UNIT_ID_H__

#include "MTSection.h"

class BaseUniverseObject;

class UnitID
{
public:
	UnitID();
	UnitID(const UnitID& unitID);
	explicit UnitID(const BaseUniverseObject* unit);
	~UnitID();

	void operator=(const UnitID& unitID) { *this = unitID.get(); }
	void operator=(const BaseUniverseObject* unit);
	void registerUnit(BaseUniverseObject* unit);
	void unregisterUnit();
	BaseUniverseObject* get() const { return map_[index_].unit; }
	int numRefs() const { return map_[index_].counter; }

	int index() const { return index_; }

	bool empty() const { return !index_; }
	bool operator == (const UnitID &id) const { return index_ == id.index_; }

	void serialize(Archive& ar);

	static void clearCounter();
	static void serializeCounter(Archive& ar);
	static bool setMergeOffset();

	static BaseUniverseObject* get(int index) { return map_[index].unit; }

private:
	struct Cell {
		BaseUniverseObject* unit;
		volatile long counter;
		int releaseQuant;
		Cell() : unit(0), counter(0), releaseQuant(0) {}
	};

	enum { MAP_SIZE = 30000, MERGE_SIZE = 6000 };
	typedef Cell Map[MAP_SIZE + MERGE_SIZE];

	int index_;
	static int totalIndex_;
	static int mergeOffset_;
	static int mapSize_;
	static Map map_;

	typedef vector<int> PackMap;
	static PackMap packMap_;
	static int packIndex_;

	void incrRef();
	void decrRef();
};

#endif //__UNIT_ID_H__
