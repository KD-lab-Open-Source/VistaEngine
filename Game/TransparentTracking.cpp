#include "StdAfx.h"
#include "Render\3dx\Node3dx.h"
#include "Render\Src\cCamera.h"
#include "TransparentTracking.h"
#include "BaseUnit.h"
#include "Universe.h"
#include "GlobalAttributes.h"

TransparentTracking::TransparentTracking()
{
	gridSize_.x = 128;
	gridSize_.y = 128;
	grid_  = new Node[gridSize_.x*gridSize_.y];
	gridFilled_ = false;
}

TransparentTracking::~TransparentTracking()
{
	delete[] grid_;
}

bool TransparentTracking::calcBound(c3dx* obj, Camera* camera, Vect2i& leftTop, Vect2i& rightBottom)
{
	start_timer_auto();

	sBox6f bound;
	obj->GetBoundBox(bound);

	bound.min.x *= GlobalAttributes::instance().opacityBoundFactorXY;
	bound.min.y *= GlobalAttributes::instance().opacityBoundFactorXY;
	bound.max.x *= GlobalAttributes::instance().opacityBoundFactorXY;
	bound.max.y *= GlobalAttributes::instance().opacityBoundFactorXY;
	bound.max.z *= GlobalAttributes::instance().opacityBoundFactorZ;

	Vect3f boundArray[8];
	boundArray[0] = bound.min;
	boundArray[1].set(bound.max.x, bound.min.y, bound.min.z);
	boundArray[2].set(bound.max.x, bound.max.y, bound.min.z);
	boundArray[3].set(bound.min.x, bound.max.y, bound.min.z);
	boundArray[4] = bound.max;
	boundArray[5].set(bound.min.x, bound.max.y, bound.max.z);
	boundArray[6].set(bound.min.x, bound.min.y, bound.max.z);
	boundArray[7].set(bound.max.x, bound.min.y, bound.max.z);
	leftTop.set(1000,1000);
	rightBottom.set(-1000,-1000);
	for(int i=0;i<8;i++){
		obj->GetPosition().xformPoint(boundArray[i]);
		Vect3f vc, vp;
		camera->matViewProj.xformCoord(boundArray[i], vc, vp);
		if(vc.z < 10)
			continue;
		int x = round((vp.x + 1.f)*0.5f*gridSize_.x);
		int y = round((vp.y + 1.f)*0.5f*gridSize_.y);

		leftTop.x = min(x, leftTop.x);
		leftTop.y = min(y, leftTop.y);
		rightBottom.x = max(x, rightBottom.x);
		rightBottom.y = max(y, rightBottom.y);
	}

	leftTop.x = max(leftTop.x, 0);
	leftTop.y = max(leftTop.y, 0);
	rightBottom.x = min(rightBottom.x, gridSize_.x - 1);
	rightBottom.y = min(rightBottom.y, gridSize_.y - 1);

	return leftTop.x <= rightBottom.x && leftTop.y <= rightBottom.y;
}

void TransparentTracking::track(Camera* camera)
{
	start_timer_auto();

	if(gridFilled_){
		gridFilled_ = false;
		for(int y = 0; y < gridSize_.y; y++)
			for(int x = 0; x < gridSize_.x; x++)
				grid_[x+y*gridSize_.x].objects.clear();
	}

	Vect3f cameraPosition = camera->GetPos();

	float opacityMin = GlobalAttributes::instance().opacityMin;
	float opacitySpeed = GlobalAttributes::instance().opacitySpeed;

	PlayerVect::iterator player;
	FOR_EACH(universe()->Players, player){
		UnitList::const_iterator unit;
		FOR_EACH((*player)->units(), unit){
			if((*unit)->canBeTransparent() && (*unit)->get3dx()){
				Vect2i leftTop, rightBottom;
				if(!calcBound((*unit)->get3dx(), camera, leftTop, rightBottom)){
					if((*unit)->position().distance2(cameraPosition) < 4000)
						(*unit)->setOpacity(max(opacityMin, (*unit)->opacity() - opacitySpeed));
					continue;
				}
				gridFilled_ = true;
				(*unit)->cameraDistance2_ = (*unit)->position().distance2(cameraPosition);
				for(int y = leftTop.y; y <= rightBottom.y; y++)
					for(int x = leftTop.x; x <= rightBottom.x; x++)
						grid_[x + y*gridSize_.x].objects.push_back(*unit);
			}
		}
	}

	if(!gridFilled_)
		return;

	UnitList::const_iterator unit;
	FOR_EACH(universe()->activePlayer()->units(), unit){
		if((*unit)->attr().isLegionary() && (*unit)->get3dx()){
			Vect2i leftTop, rightBottom;
			if(!calcBound((*unit)->get3dx(), camera, leftTop, rightBottom))
				continue;
			float cameraDistance2 = (*unit)->position().distance2(cameraPosition);
			for(int y = leftTop.y; y <= rightBottom.y; y++)
				for(int x = leftTop.x; x <= rightBottom.x; x++){
					Node& node = grid_[x + y*gridSize_.x];
					Objects::iterator unit2;
					FOR_EACH(node.objects, unit2){
						if(cameraDistance2 > (*unit2)->cameraDistance2_){
							(*unit2)->setOpacity(max(opacityMin, (*unit2)->opacity() - opacitySpeed));
							(*unit2)->cameraDistance2_ = FLT_INF;
						}
					}
				}
		}
	}
}

