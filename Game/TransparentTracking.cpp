#include "StdAfx.h"
#include "..\Render\3dx\Node3dx.h"
#include "TransparentTracking.h"
#include "BaseUnit.h"
cTransparentTracking::cTransparentTracking()
{
	grid_size.x = 64;
	grid_size.y = 64;
	grid  = new Node[grid_size.x*grid_size.y];
	quant_num = 0;
}

cTransparentTracking::~cTransparentTracking()
{
//	UnitContainer::iterator it;
//	FOR_EACH(transparent, it)
//		if (it->unit->model())
//			it->unit->model()->SetColor(NULL, &sColor4f(1,1,1,1));
	delete[] grid;
}


inline void obrez(int& t, const int min, const int max)
{
	if (t<min) t = min;
	if (t>=max) t = max;
}

void cTransparentTracking::Tracking(cCamera *pCamera)
{
	Vect3f p[8];
	const Vect3f& camera_pos = pCamera->GetPos();
	pCamera->GetFrustumPoint(p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);

	Vect2f cell_size;
	Vect3f Ox = p[6]-p[4]; 
	cell_size.x = grid_size.x/Ox.norm();
	FastNormalize(Ox);
	Vect3f Oy = p[5]-p[4]; 
	cell_size.y = grid_size.y/Oy.norm();
	FastNormalize(Oy);
	Vect3f Oz = Ox%Oy;
	Vect3f O = p[4];
	Mat3f transfer(	Ox.x, Oy.x, Oz.x,
					Ox.y, Oy.y, Oz.y,
					Ox.z, Oy.z, Oz.z);
	MatXf to_plane(transfer, O);
	sPlane4f far_plane(O, O+Ox, O+Oy);


	UnitContainer::iterator it;
	FOR_EACH(transparent, it)
	{
		cObject3dx* obj = it->unit->model();
		xassert(0);
		if (obj && false/*obj->GetAttr(ATTRUNKOBJ_VISIBLE)*/)
		{
			sBox6f bound;
			obj->GetBoundBox(bound);

			Vect3f bd[8];
			bd[0] = bound.min;
			bd[1].set(bound.max.x, bound.min.y, bound.min.z);
			bd[2].set(bound.max.x, bound.max.y, bound.min.z);
			bd[3].set(bound.min.x, bound.max.y, bound.min.z);
			bd[4] = bound.max;
			bd[5].set(bound.min.x, bound.max.y, bound.max.z);
			bd[6].set(bound.min.x, bound.min.y, bound.max.z);
			bd[7].set(bound.max.x, bound.min.y, bound.max.z);
			Vect2i bdi[8];
			for(int i=0;i<8;i++)
			{
				bd[i] = obj->GetPosition()*bd[i];
				float t = far_plane.GetCross(camera_pos, bd[i]);
				bd[i] = (bd[i] - camera_pos)*t + camera_pos;
				to_plane.invXformPoint(bd[i]);
				bdi[i].x = round(bd[i].x*cell_size.x);
				bdi[i].y = round(bd[i].y*cell_size.y);
			}
			
			Vect2i min(1000,1000), max(-1,-1);
			for(int i=0;i<8;i++)
			{
				if (bdi[i].x<min.x) min.x = bdi[i].x;
				if (bdi[i].y<min.y) min.y = bdi[i].y;

				if (bdi[i].x>max.x) max.x = bdi[i].x;
				if (bdi[i].y>max.y) max.y = bdi[i].y;
			}
			obrez(min.x, 0,grid_size.x);
			obrez(max.x, 0,grid_size.x);
			obrez(min.y, 0,grid_size.y);
			obrez(max.y, 0,grid_size.y);
			it->camera_distance2 = obj->GetPosition().trans().distance2(camera_pos);
			it->need_transparent = false; 
			for(int y=min.y; y<max.y; y++)
			for(int x=min.x; x<max.x; x++)
			{
				Node &node = grid[x+y*grid_size.x];
				if (node.quant_num!=quant_num)
				{
					node.quant_num = quant_num;
					node.objs.clear();
				}
				node.objs.push_back(&(*it));
			}
		}
	}
	FOR_EACH(equitant, it)
	{
		cObject3dx* obj = it->unit->model();
		xassert(0);
		if (obj&& false/*obj->GetAttr(ATTRUNKOBJ_VISIBLE)*/)
		{
			sBox6f bound;
			obj->GetBoundBox(bound);
			Vect2i min,max;
			bound.min = obj->GetPosition()*(bound.min);
			bound.max = obj->GetPosition()*(bound.max);
			float t = far_plane.GetCross(camera_pos, bound.min);
			bound.min = (bound.min - camera_pos)*t + camera_pos;
			t = far_plane.GetCross(camera_pos, bound.max);
			bound.max = (bound.max - camera_pos)*t + camera_pos;
			to_plane.invXformPoint(bound.min);
			to_plane.invXformPoint(bound.max);
			if (fabsf(bound.min.z)>1e-3f || fabsf(bound.max.z)>1e-3f)
				it=it;
			min.x = (round(bound.min.x*cell_size.x));
			min.y = (round(bound.min.y*cell_size.y));
			max.x = (round(bound.max.x*cell_size.x));
			max.y = (round(bound.max.y*cell_size.y));
			if (min.x>max.x)
				swap(min.x, max.x);
			if (min.y>max.y)
				swap(min.y, max.y);
			obrez(min.x, 0,grid_size.x-1);
			obrez(max.x, 0,grid_size.x-1);
			obrez(min.y, 0,grid_size.y-1);
			obrez(max.y, 0,grid_size.y-1);
			float dist2 = obj->GetPosition().trans().distance2(camera_pos);
			for(int y=min.y; y<=max.y; y++)
			for(int x=min.x; x<=max.x; x++)
			{
				vector<TransparentObj*>::iterator ittr;
				xassert((UINT)x<grid_size.x && (UINT)y<grid_size.y);
				Node &node = grid[x+y*grid_size.x];
				if (node.quant_num==quant_num)
				FOR_EACH(node.objs, ittr)
				{
					TransparentObj* tobj = *ittr;
					if (!tobj->need_transparent && dist2>(tobj->camera_distance2))
					{
						tobj->unit->setOpacity(0.2f);
						tobj->need_transparent = true;
					}
				}
			}
		}
	}
	FOR_EACH(transparent, it)
	{
		if (!it->need_transparent && it->unit->opacity()<0.999f)
		{
			it->unit->setOpacity(1.0f);
		}
	}
	quant_num++;
}

cTransparentTracking::TransparentObj::TransparentObj(UnitBase* pUnit):already_transparent(false), unit(pUnit)
{

}
inline bool isEquitant(UnitBase* unit)
{
	return unit->attr().isLegionary();
}

void cTransparentTracking::RegisterUnit(UnitBase* unit)
{
	UnitContainer::iterator it;
	if (unit->transparentMode())
	{
		TransparentObj obj(unit);
		if (transparent.empty())
			transparent.push_back(obj);
		else
		{
			it = lower_bound(transparent.begin(), transparent.end(), obj);
			transparent.insert(it, obj);
		}
//		FOR_EACH(transparent, it)
//			if (it->unit==unit)
//				return;
//		transparent.push_back(TransparentObj(unit));
	}
	else if (isEquitant(unit))
	{
		TransparentObj obj(unit);
		if (equitant.empty())
			equitant.push_back(obj);
		else
		{
			it = lower_bound(equitant.begin(), equitant.end(), obj);
			equitant.insert(it, obj);
		}
	}
}


void cTransparentTracking::UnRegisterUnit(UnitBase* unit)
{
	UnitContainer::iterator it;
	if (unit->transparentMode())
	{
		it = lower_bound(transparent.begin(), transparent.end(), TransparentObj(unit));
		if (it!=transparent.end())
		{
			xassert(it->unit == unit);
			transparent.erase(it);
		}
	}
	else if (isEquitant(unit))
	{
		it = lower_bound(equitant.begin(), equitant.end(), TransparentObj(unit));
		if (it!=equitant.end())
		{
			xassert(it->unit == unit);
			equitant.erase(it);
		}
	}
}


void cTransparentTracking::RewriteUnit(UnitBase* unit, bool prev_tranparent)
{
	UnitContainer::iterator it;
	if (prev_tranparent)
	{
		it = lower_bound(transparent.begin(), transparent.end(), TransparentObj(unit));
		if (it!=transparent.end())
		{
			xassert(it->unit == unit);
			transparent.erase(it);
		}
	}
	else if (isEquitant(unit))
	{
		it = lower_bound(equitant.begin(), equitant.end(), TransparentObj(unit));
		if (it!=equitant.end())
		{
			xassert(it->unit == unit);
			equitant.erase(it);
		}
	}
	RegisterUnit(unit);
}
