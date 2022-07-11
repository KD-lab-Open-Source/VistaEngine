#include "StdAfx.h"
#include "UnitID.h"
#include "BaseUnit.h"
#include "Universe.h"
#include "Windows.h"

#pragma comment(lib)

const int RELEASE_DELAY_QUANTS = 1200;
int UnitID::totalIndex_ = 1;
int UnitID::mergeOffset_ = 0;
int UnitID::mapSize_ = UnitID::MAP_SIZE;
UnitID::Map UnitID::map_;
UnitID::PackMap UnitID::packMap_;
int UnitID::packIndex_ = 0;

struct UnitIdInitializer
{
	UnitIdInitializer() { UnitID::clearCounter(); }
};

static UnitIdInitializer unitIdInitializer;

UnitID::UnitID() 
{ 
	index_ = 0;
}

UnitID::UnitID(const UnitID& unitID) 
{
	index_ = unitID.index_;
	if(index_)
		incrRef();
}

UnitID::UnitID(const BaseUniverseObject* unit) 
{
	index_ = 0;
	*this = unit;
}

inline void UnitID::incrRef() 
{
	InterlockedIncrement(&map_[index_].counter);
}

inline void UnitID::decrRef() 
{
	InterlockedDecrement(&map_[index_].counter);
#ifndef _FINAL_VERSION_
	if(!MT_IS_LOGIC() && !map_[index_].unit && map_[index_].releaseQuant < universe()->quantCounter())
		xassert(0);
#endif
}

void UnitID::operator=(const BaseUniverseObject* unit)
{
	if(index_){
		decrRef();
		index_ = 0;
	}
	
	if(unit){
		index_ = unit->unitID().index();
		if(index_)
			incrRef();
	}
}

UnitID::~UnitID()
{
	if(index_){
		decrRef();
		index_ = 0;
	}
}

void UnitID::registerUnit(BaseUniverseObject* unit)
{
	MTL();
	if(map_[index_].unit){
		xassert(0 && "Уникальный ид уже занят, нужно перезаписать этот мир");
		index_ = 0;
	}

	if(!index_){
		bool forceRealloc = false;
		for(;; ++totalIndex_){
			if(totalIndex_ >= mapSize_){
				if(!forceRealloc){
					totalIndex_ = 1;
					forceRealloc = true; 
				}
				else{
					xassert(0 && "Не хватает unitID");
					return;
				}
			}
			
			Cell& cell = map_[totalIndex_];
			//log_var((bool)cell.unit);
			//log_var(cell.releaseQuant);
			if(!cell.unit && cell.releaseQuant <= universe()->quantCounter()){
				//log_var(totalIndex_);
				//log_var(cell.counter);
				if(!cell.counter)
					break;
			}
		}
		index_ = totalIndex_++;
	}
	xassert(!map_[index_].unit && "Уникальный ид уже занят, нужно перезаписать этот мир");
	
	incrRef();
	map_[index_].unit = unit;
	log_var(index_);

#ifndef _FINAL_VERSION_
	int counter1 = 0;
	int counter2 = 0;
	for(int i = 0; i < mapSize_; i++)
		if(map_[i].unit)
			counter1++;
		else if(map_[i].releaseQuant > universe()->quantCounter())
			counter2++;
	statistics_add(counter1, counter1);
	statistics_add(counter2, counter2);
#endif
}

void UnitID::unregisterUnit()
{
	MTL();
	//log_var(index_);
	if(index_){
		map_[index_].unit = 0;
		decrRef();
		map_[index_].releaseQuant = universe()->quantCounter() + RELEASE_DELAY_QUANTS;
		index_ = 0;
	}
}

void UnitID::serialize(Archive& ar) 
{
	if(ar.isOutput()){
		int index = 0;
		if(get()){
			if(packIndex_){
				if(!packMap_[index_])
					packMap_[index_] = packIndex_++;
				index = packMap_[index_];
			}
			else
				index = index_;
		}
		ar.serialize(index, "unitID", 0);
	}
	else{
		if(index_)
			decrRef();

		ar.serialize(index_, "unitID", 0);

		if(index_)
			index_ += mergeOffset_;
	
		if(index_){
			xassert(index_ < mapSize_);
			incrRef();
		}
	}
}

void UnitID::clearCounter()
{
	totalIndex_ = 1;
	mapSize_ = MAP_SIZE;
	memset(map_, 0, (MAP_SIZE + MERGE_SIZE)*sizeof(Cell));

	packIndex_ = 0;
	packMap_.clear();

	mergeOffset_ = 0;
}

void UnitID::serializeCounter(Archive& ar)
{
	int totalCounter = 0;
	if(ar.isOutput()){
		for(int i = 0; i < mapSize_; i++)
			if(map_[i].unit)
				totalCounter++;
	}
	ar.serialize(totalCounter, "totalCounter", 0);

	if(ar.isInput()){
		totalIndex_ = totalCounter + 1;
		if(totalIndex_ > mapSize_){
			mapSize_ = MAP_SIZE + MERGE_SIZE;
			totalIndex_ = MAP_SIZE;
		}
		packIndex_ = 0;
	}
	else{
		packIndex_ = 1;
        packMap_.clear();
		packMap_.insert(packMap_.end(), mapSize_, 0);
	}
}

bool UnitID::setMergeOffset()
{
	if(mergeOffset_){
		xassert(0 && "Допускается только одна операция импорта, затем мир нужно записать и перезагрудить. Старые миры необходимо вначале перезаписать");
		return false;
	}
	mapSize_ = MAP_SIZE + MERGE_SIZE;
	mergeOffset_ = MAP_SIZE;
	return true;
}

