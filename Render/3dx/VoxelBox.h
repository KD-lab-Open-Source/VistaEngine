#pragma once

#include "Umath.h"
#include "Serialization\Serialization.h"

class cObject3dx;
struct Color4c;
class Archive;

class VoxelBox
{
public:
	VoxelBox(int sizeLen);
	~VoxelBox();
	void create(cObject3dx* model);
	void add(const Vect3f& pos);
	bool check(const Vect3f& pos);
	bool trace(const Vect3f& start, const Vect3f& end, Vect3f& collision) const;
	void serialize(Archive& ar);
	bool valid() const { return valid_; }

	void draw(const Vect3f& pos, Color4c color);

private:
	bool valid_;
	int sizeLen_;
	int size_;
	int mask_;
	Vect3f scale_;
	Vect3f scaleInv_;
	Vect3f offset_;
	MemoryBlock buffer_;

	void addTriangleRecursive(const Vect3f& p0, const Vect3f& p1, const Vect3f& p2);
	
public:
	Vect3f convert(const Vect3f& v) const { 
		Vect3f vc = (v - offset_)*scale_; 
		xassert(vc.xi() >= 0 && vc.xi() < size_ && vc.yi() >= 0 && vc.yi() < size_ && vc.zi() >= 0 && vc.zi() < size_);
		return vc;
	}
	Vect3f convertInv(const Vect3f& v) const { return v*scale_ + offset_; }
};