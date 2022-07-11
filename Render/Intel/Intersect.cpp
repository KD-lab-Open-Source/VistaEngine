#include "StdAfxRD.h"
#include "..\MathSSE\_vector3_sse.h"
#include "..\3dx\node3dx.h"
#include "..\inc\SmallCache.h"

class MatXfsse
{
public:
	__m128 x;
	__m128 y;
	__m128 z;
	__m128 w;

public:
	void set(MatXf& m)
	{
		x=_mm_set_ps(0,m.R.zx,m.R.yx,m.R.xx);
		y=_mm_set_ps(0,m.R.zy,m.R.yy,m.R.xy);
		z=_mm_set_ps(0,m.R.zz,m.R.yz,m.R.xz);
		w=_mm_set_ps(0,m.d.z,m.d.y,m.d.x);

//		x.set(m.R.xx,m.R.yx,m.R.zx,0);
//		y.set(m.R.xy,m.R.yy,m.R.zy,0);
//		z.set(m.R.xz,m.R.yz,m.R.zz,0);
//		w.set(m.d.x,m.d.y,m.d.z,0);
	}

	void set_mul(MatXfsse& m,float f)
	{
		__m128 scale = _mm_set_ps1(f);
		x = _mm_mul_ps(m.x, scale);
		y = _mm_mul_ps(m.y, scale);
		z = _mm_mul_ps(m.z, scale);
		w = _mm_mul_ps(m.w, scale);
	}

	void mad(MatXfsse& m,float f)
	{
		__m128 scale = _mm_set_ps1(f);
		x = _mm_add_ps(x,_mm_mul_ps(m.x, scale));
		y = _mm_add_ps(y,_mm_mul_ps(m.y, scale));
		z = _mm_add_ps(z,_mm_mul_ps(m.z, scale));
		w = _mm_add_ps(w,_mm_mul_ps(m.w, scale));
	}
};

_vector3_sse operator* (const MatXfsse& m,const _vector3_sse& v)
{
    return _vector3_sse(
        _mm_add_ps(
		_mm_add_ps(
		_mm_add_ps(
			_mm_mul_ps(m.x, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0))), 
			_mm_mul_ps(m.y, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1)))), 
			_mm_mul_ps(m.z, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2)))), 
			m.w)
    );
}

/*
_vector3_sse cross(const _vector3_sse v0,const _vector3_sse v1)
{
    static const int X = 0;
    static const int Y = 1;
    static const int Z = 2;
    static const int W = 3;

    __m128 a = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(W, X, Z, Y));
    __m128 b = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(W, Y, X, Z));
    __m128 c = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(W, Y, X, Z));
    __m128 d = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(W, X, Z, Y));

    __m128 e = _mm_mul_ps(a, b);
    __m128 f = _mm_mul_ps(c, d);

    return _vector3_sse(_mm_sub_ps(e, f));
}
*/

bool IntersectionRay(const _vector3_sse& a,const _vector3_sse& b,const _vector3_sse& c,
		const _vector3_sse& p0,const _vector3_sse& pn)
{
	_vector3_sse nab=cross((b-a),pn),nbc=cross((c-b),pn),nca=cross((a-c),pn);
	float kab=dot(nab,(p0-a));
	float kbc=dot(nbc,(p0-b));
	float kca=dot(nca,(p0-c));
	return (kab<0 && kbc<0 && kca<0)||(kab>0 && kbc>0 && kca>0);
}

__forceinline
_vector3_sse cross2(const _vector3_sse& v0, const _vector3_sse& b, const _vector3_sse& d) 
{
    // x = v0.y * v1.z - v0.z * v1.y
    // y = v0.z * v1.x - v0.x * v1.z
    // z = v0.x * v1.y - v0.y * v1.x
    //
    // a = v0.y | v0.z | v0.x | xxx
    // b = v1.z | v1.x | v1.y | xxx
    // c = v0.z | v0.x | v0.y | xxx
    // d = v1.y | v1.z | v1.x | xxx
    //

    static const int X = 0;
    static const int Y = 1;
    static const int Z = 2;
    static const int W = 3;

    __m128 a = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(W, X, Z, Y));
    __m128 c = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(W, Y, X, Z));

    __m128 e = _mm_mul_ps(a, b);
    __m128 f = _mm_mul_ps(c, d);

    return _vector3_sse(_mm_sub_ps(e, f));
}

__forceinline
_vector3_sse dot3(const _vector3_sse& a0,const _vector3_sse& b0,const _vector3_sse& c0,
				  const _vector3_sse& aa,const _vector3_sse& bb,const _vector3_sse& cc)
{
	__m128 a = _mm_mul_ps(a0, aa);
	__m128 b = _mm_mul_ps(b0, bb);
	__m128 c = _mm_mul_ps(c0, cc);

	//o.x=a.x+a.y+a.z;
	//o.y=b.x+b.y+b.z;
	//o.z=c.x+c.y+c.z;

//	_mm_shuffle_ps(a, c, _MM_SHUFFLE(0,0,0,0))/*ax,X,cx,X*/
//	_mm_shuffle_ps(a, c, _MM_SHUFFLE(0,1,0,1))/*ay,X,cy,X*/
//	_mm_shuffle_ps(a, c, _MM_SHUFFLE(0,2,0,2))/*az,X,cz,X*/

//	_mm_unpacklo_ps(a,b)//ax,bx,ay,by
//	_mm_unpackhi_ps(a,b)//az,bz,aw,bw
//	_mm_movehl_ps(a,b)//aw,ay,bw,by
//	_mm_movelh_ps(a,b)//by,bx,ay,ax
	//__m128 lh=_mm_movelh_ps(a,b);//ax,ay,bx,by
	__m128 abxy=_mm_unpacklo_ps(a,b);/*ax,bx,ay,by*/
	__m128 abyx=_mm_movehl_ps(abxy,abxy);/*ay,by,ay,by*/

	__m128 sx=_mm_shuffle_ps(abxy,c,_MM_SHUFFLE(3,0,1,0))/*ax,bx,cx,cw*/;
	__m128 sy=_mm_shuffle_ps(abyx,c,_MM_SHUFFLE(3,1,1,0))/*ay,by,cy,cw*/;
	__m128 sz=_mm_shuffle_ps(_mm_unpackhi_ps(a,b)/*az,bz,aw,bw*/,c,_MM_SHUFFLE(3,2,1,0))/*az,bz,cz,cw*/;

	
	return _vector3_sse(_mm_add_ps(sx,_mm_add_ps(sy,sz)));

}

__forceinline
bool IntersectionRay2(const _vector3_sse& a,const _vector3_sse& b,const _vector3_sse& c,
		const _vector3_sse& p0,const _vector3_sse& pn)
{
    static const int X = 0;
    static const int Y = 1;
    static const int Z = 2;
    static const int W = 3;
    __m128 pn_b = _mm_shuffle_ps(pn, pn, _MM_SHUFFLE(W, Y, X, Z));
    __m128 pn_d = _mm_shuffle_ps(pn, pn, _MM_SHUFFLE(W, X, Z, Y));

	_vector3_sse nab=cross2((b-a),pn_b,pn_d),nbc=cross2((c-b),pn_b,pn_d),nca=cross2((a-c),pn_b,pn_d);
/* //v1
	float kab=dot(nab,(p0-a));
	float kbc=dot(nbc,(p0-b));
	float kca=dot(nca,(p0-c));

	return (kab<0 && kbc<0 && kca<0)||(kab>0 && kbc>0 && kca>0);
/*/
//* //v2
	_vector3_sse kabc=dot3(nab,nbc,nca,p0-a,p0-b,p0-c);
	int mask=_mm_movemask_ps(kabc);
	return mask==0 || mask==7;
/**/
}

//p0=(0,0,0), pn=(0,0,1)
/*
bool IntersectionRaySpecial(const _vector3_sse& a,const _vector3_sse& b,const _vector3_sse& c)
{
	float kab=(b.y-a.y)*a.x-(b.x-a.x)*a.y;
	float kbc=(c.y-b.y)*b.x-(c.x-b.x)*b.y;
	float kca=(a.y-c.y)*c.x-(a.x-c.x)*c.y;
	return (kab<0 && kbc<0 && kca<0)||(kab>0 && kbc>0 && kca>0);
}
/**/
//*

//!!! некорректно работает для вырожденных треугольников, возвращает для них всегда true
bool IntersectionRaySpecial(const _vector3_sse& a,const _vector3_sse& b,const _vector3_sse& c)
{
	//sx=(a.x,b.x,c.x,0)
	//sy=(a.y,b.y,c.y,0)
	//ix=(b.x,c.x,a.x,0)
	//iy=(b.y,c.y,a.y,0)
	//(iy-sy)*sx-(ix-sx)*sy

	__m128 abxy=_mm_unpacklo_ps(a,b);//ax,bx,ay,by
	__m128 abyx=_mm_movehl_ps(abxy,abxy);//ay,by,ay,by

	__m128 sx=_mm_shuffle_ps(abxy,c,_MM_SHUFFLE(3,0,1,0));//ax,bx,cx,cw
	__m128 sy=_mm_shuffle_ps(abyx,c,_MM_SHUFFLE(3,1,1,0));//ay,by,cy,cw

	__m128 ix=_mm_shuffle_ps(sx,sx,_MM_SHUFFLE(3,0,2,1));//b,c,a
	__m128 iy=_mm_shuffle_ps(sy,sy,_MM_SHUFFLE(3,0,2,1));//b,c,a

	__m128 kabc=_mm_sub_ps(
	_mm_mul_ps(_mm_sub_ps(iy,sy),sx),
	_mm_mul_ps(_mm_sub_ps(ix,sx),sy));
	int mask=_mm_movemask_ps(kabc);
	return mask==0 || mask==7;
}
/**/
//*
bool cObject3dx::IntersectTriangleSSE(const Vect3f& p0,const Vect3f& p1) const
{
	pStatic->CacheBuffersToSystemMem(iLOD);
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	SmallCache<_vector3_sse,5> cache;
	MatXf offset;
	CalcOffsetMatrix(offset,p0,p1);
//	Vect3f pn=p1-p0;
//	_vector3_sse pnorm(pn.x,pn.y,pn.z);
//	pnorm.norm();
//	_vector3_sse p0_sse(p0.x,p0.y,p0.z);
	bool intersect=false;

	int blend_weight=lod.GetBlendWeight();
	cSkinVertexSysMemI skin_vertex;
	sPolygon* pPolygon = lod.sys_ib;
	skin_vertex.SetVB(lod.sys_vb);
	float int2float=1/255.0f;

	for(int isg=0;isg<lod.skin_group.size();isg++)
	{
		cStaticIndex& s=lod.skin_group[isg];
		if(!IsVisibleMaterialGroup(s))
			continue;

		const MaterialAnim& mat_anim=materials[s.imaterial];
		float alpha=mat_anim.opacity*object_opacity*distance_alpha;
		if(alpha<BLEND_STATE_ALPHA_REF/255.0f)
			continue;

		MatXf world[cStaticIndex::max_index];
		int world_num;
		GetWorldPos(s,world,world_num);
		for(int iworld=0;iworld<world_num;iworld++)
			world[iworld]=offset*world[iworld];

		MatXfsse world_sse[cStaticIndex::max_index];
		for(int world_index=0;world_index<world_num;world_index++)
			world_sse[world_index].set(world[world_index]);

		//void cObject3dx::GetWorldPos(cStaticIndex& s,MatXf* world,int& world_num)
		//void cObject3dx::GetWorldPos(cStaticIndex& s,MatXf& world, int& idx)

		for(int ivg=0;ivg<s.visible_group.size();ivg++)
		{
			cTempVisibleGroup& vg=s.visible_group[ivg];
			if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
			{
				int begin_polygon=s.offset_polygon+vg.begin_polygon;
				int end_polygon=begin_polygon+vg.num_polygon;
				for(int ipolygon=begin_polygon;ipolygon<end_polygon;ipolygon++)
				{
					sPolygon& polygon=pPolygon[ipolygon];
					_vector3_sse* real_pos[3];
					for(int ivertex=0;ivertex<3;ivertex++)
					{
						int vertex=polygon[ivertex];
						_vector3_sse* pos=cache.get(vertex);
						if(pos==NULL)
						{
							skin_vertex.Select(vertex);

							_vector3_sse global_pos;
							Vect3f& pos3=skin_vertex.GetPos();
							_vector3_sse cur_pos(pos3.x,pos3.y,pos3.z);
							if(lod.blend_indices==1)
							{
								int idx=skin_vertex.GetIndex()[0];
								global_pos=world_sse[idx]*cur_pos;
							}else
							{
								global_pos.set(0,0,0);
								MatXfsse temp;
								temp.set_mul(world_sse[skin_vertex.GetIndex()[0]],skin_vertex.GetWeight(0)*int2float);
								for(int ibone=1;ibone<blend_weight;ibone++)
								{
									int idx=skin_vertex.GetIndex()[ibone];
									float weight=skin_vertex.GetWeight(ibone)*int2float;
									temp.mad(world_sse[idx],weight);
								}
								global_pos=temp*cur_pos;
							}

							pos=cache.add(vertex,global_pos);
//							gb_RenderDevice3D->DrawPoint(*pos,sColor4c(255,255,255,255));
						}

						real_pos[ivertex]=pos;
					}

					//if(IntersectionRay2(*real_pos[0],*real_pos[1],*real_pos[2],p0_sse,pnorm))
					if(IntersectionRaySpecial(*real_pos[0],*real_pos[1],*real_pos[2]))
					{
						intersect=true;
						return true;//!!!!!!Only if lock/unlock not needed.
					}
				}
			}
		}
	}

	return intersect;
}
/**/
