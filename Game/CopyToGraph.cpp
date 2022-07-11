#include "StdAfx.h"
#include "ForceField.h"
#include "RenderObjects.h"
#include "terra.h"
#include "..\Render\src\MultiRegion.h"
#include "BaseUnit.h"
#include "TerraInterface.inl"
#include "..\physics\windMap.h"
#include "NumDetailTexture.h"

class DetailTerraInterface:public StandartTerraInterface
{
	MultiRegion region_; 
	MTSection lock_;
public:
	DetailTerraInterface()
		:region_(vMap.H_SIZE,vMap.V_SIZE,1)
	{
	}
	virtual ~DetailTerraInterface()
	{
	}

	class MultiRegion* GetRegion()
	{
		xassert(lock_.locked());
		return &region_;
	}	

	const Vect2f& GetWindVelocity(const int x, const int y)
	{
		if (windMap && windMap->lookable(x, y))
			return windMap->look(x, y);
		return Vect2f::ZERO;
	}
	
	void LockColumn(){
		lock_.lock();
	}
	void UnlockColumn(){
		lock_.unlock();
	}
};

MultiRegion* TileMapRegion()
{
	return static_cast<DetailTerraInterface*>(terMapPoint->GetTerra())->GetRegion();
}

void DrawOnRegion(int layer, const Vect2i& point, float radius)
{
	terMapPoint->GetTerra()->LockColumn();
	int num = terMapPoint->GetZeroplastNumber();
	xassert((UINT)layer<num);

	ShapeRegion circle;
	circle.circle(radius,layer+1);
	circle.move(point.x,point.y);
	TileMapRegion()->operate(circle);

	terMapPoint->GetTerra()->UnlockColumn();
}

TerraInterface* CreateTerraInterface()
{
	return new DetailTerraInterface;
}

