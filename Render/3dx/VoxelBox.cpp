#include "stdafx.h"
#include "VoxelBox.h"
#include "Render\3dx\Node3DX.h"

VoxelBox::VoxelBox(int sizeLen)
{
	valid_ = false;
	sizeLen_ = sizeLen;
	size_ = 1 << sizeLen;
	mask_ = size_ - 1;
	buffer_.alloc(size_*size_*size_/8);

	scale_ = Vect3f::ZERO;
	offset_ = Vect3f::ZERO;
}

VoxelBox::~VoxelBox()
{
}

void VoxelBox::create(cObject3dx* model)
{
	valid_ = true;
	memset(buffer_.buffer(), 0, buffer_.size());

	TriangleInfo info;
	model->GetTriangleInfo(info,TIF_TRIANGLES|TIF_POSITIONS|TIF_ZERO_POS);

	sBox6f bound;
	vector<Vect3f>::iterator vi;
	FOR_EACH(info.positions, vi)
		bound.addPoint(*vi);

	offset_ = bound.min;
	Vect3f delta = bound.max - bound.min;
	delta.x = max(delta.x, 1e-3f);
	delta.y = max(delta.y, 1e-3f);
	delta.z = max(delta.z, 1e-3f);
	scale_ = Vect3f(mask_, mask_, mask_)/delta;
	scaleInv_ = delta/Vect3f(mask_, mask_, mask_);

	vector<sPolygon>::const_iterator pi;
	FOR_EACH(info.triangles, pi)
		addTriangleRecursive(info.positions[pi->p1], info.positions[pi->p2], info.positions[pi->p3]);
}

void VoxelBox::addTriangleRecursive(const Vect3f& p0, const Vect3f& p1, const Vect3f& p2)
{
	Vect3f center = (p0 + p1 + p2)/3;
	add(center);
	
	const float error2 = sqr(0.5f*scaleInv_.maxAbs());

	float d0 = p0.distance2(p1);
	float d1 = p1.distance2(p2);
	float d2 = p2.distance2(p0);
	if(d0 > d1)
		if(d0 > d2){ // d0 - max
			if(d0 > error2){
				Vect3f pm = (p0 + p1)/2;
				addTriangleRecursive(p0, pm, p2);
				addTriangleRecursive(p1, pm, p2);
			}
			return;
		}
	else
		if(d1 > d2) { // d1 - max
			if(d1 > error2){
				Vect3f pm = (p1 + p2)/2;
				addTriangleRecursive(p1, pm, p0);
				addTriangleRecursive(p2, pm, p0);
			}
			return;
		}

	// d2 - max

	if(d2 > error2){
		Vect3f pm = (p2 + p0)/2;
		addTriangleRecursive(p2, pm, p1);
		addTriangleRecursive(p0, pm, p1);
	}
}

void VoxelBox::add(const Vect3f& pos)
{
	Vect3f posLocal = convert(pos);
	int index = (posLocal.xi() & mask_) + ((posLocal.yi() & mask_) << sizeLen_) + ((posLocal.zi() & mask_) << 2*sizeLen_);
	int bit = index & 7;
	index >>= 3;
	buffer_.buffer()[index] |= 1 << bit;
}

bool VoxelBox::check(const Vect3f& pos)
{
	Vect3f posLocal = convert(pos);
	int index = (posLocal.xi() & mask_) + ((posLocal.yi() & mask_) << sizeLen_) + ((posLocal.zi() & mask_) << 2*sizeLen_);
	int bit = index & 7;
	index >>= 3;
	return buffer_.buffer()[index] & (1 << bit);
}

bool VoxelBox::trace(const Vect3f& start, const Vect3f& end, Vect3f& collision) const
{
	xassert(valid());

	Vect3f p0 = (start - offset_)*scale_; 
	Vect3f p1 = (end - offset_)*scale_; 

	Vect3f dir = p1 - p0;

	// Box clipping
	float t0 = 0;
	float t1 = 1;
	for(int i = 0; i < 3; ++i){
		if(dir[i] > 0){
			if(p0[i] > mask_ || p1[i] < 0)
				return false;
			if(dir[i] > FLT_EPS) {
				t0 = max(t0, -p0[i]/dir[i]);
				t1 = min(t1, (mask_ - p0[i])/dir[i]);
			}
		} 
		else{
			if(p1[i] > mask_ || p0[i] < 0)
				return false;
			if(dir[i] < -FLT_EPS) {
				t0 = max(t0, (mask_ - p0[i])/dir[i]);
				t1 = min(t1, -p0[i]/dir[i]);
			}
		}
	}

	if(t1 < t0)
		return false;

	p1 = p0 + dir*t1;
	p0 += dir*t0;

	// Scan raster
	Vect3f delta = p1 - p0;
	float maxVal = delta.maxAbs();
	int steps = ceilf(maxVal);
	delta /= max(steps, 1);
	while(steps--){
		xassert(p0.xi() >= 0 && p0.xi() < size_ && p0.yi() >= 0 && p0.yi() < size_ && p0.zi() >= 0 && p0.zi() < size_);
		int index = (p0.xi() & mask_) + ((p0.yi() & mask_) << sizeLen_) + ((p0.zi() & mask_) << 2*sizeLen_);
		int bit = index & 7;
		index >>= 3;
		if(buffer_.buffer()[index] & (1 << bit)){
			collision = p0*scaleInv_ + offset_;
			return true;
		}
		p0 += delta;
	}

	return false;
}

void VoxelBox::serialize(Archive& ar)
{
	ar.serialize(scale_, "scale", "scale");
	ar.serialize(scaleInv_, "scaleInv", "scaleInv");
	ar.serialize(offset_, "offset", "offset");
	ar.serialize(buffer_, "buffer", "buffer");
}

void VoxelBox::draw(const Vect3f& pos, Color4c color)
{
	for(int z = 0; z < size_; z++)
		for(int y = 0; y < size_; y++)
			for(int x = 0; x < size_; x++){
				int index = x + (y << sizeLen_) + (z << 2*sizeLen_);
				int bit = index & 7;
				index >>= 3;
				if(buffer_.buffer()[index] & (1 << bit)){
					Vect3f v(x, y, z);
					v *= scaleInv_;
					v += offset_;
					v += pos;
					gb_RenderDevice->DrawPoint(v, color);
				}
			}
}
