#include "StdAfxRd.h"
#include "FogOfWar.h"
#include "..\inc\TerraInterface.inl"
#include <emmintrin.h>		// MMX, SSE, SSE2 intrinsic support
#include "Serialization.h"
#include "DebugUtil.h"


///////////////////////////////////////////////////////

class cOneCirceData
{
public:
	enum
	{
		quality=2,//Два бита меньше размера сетки
		quality_mul=1<<quality,
		quality_and=quality_mul-1,
	};

	Vect2i size;
	Vect2i offset;
	Vect2i center;
	char* data;

	cOneCirceData(int radius,int delta_radius,Vect2i offset_);

	~cOneCirceData()
	{
		delete[] data;
	}
};

class cCircleDraw
{
	vector<cOneCirceData*> circles;
	int max_radius;
	int shift;
	int quality_shift;

	cOneCirceData* Get(int radius,int x,int y)
	{
		xassert(radius>0 && radius<=max_radius);
		x=x>>quality_shift;
		y=y>>quality_shift;
		x&=cOneCirceData::quality_and;
		y&=cOneCirceData::quality_and;

		int qmul=cOneCirceData::quality_mul;
		int offset=radius*qmul*qmul+x+y*qmul;
		xassert(offset>=0 && offset<circles.size());
		return circles[offset];
	}
public:
	cCircleDraw(int max_radius_,int shift_);
	~cCircleDraw()
	{
		vector<cOneCirceData*>::iterator it;
		FOR_EACH(circles,it)
		{
			delete *it;
		}
	}
	void Draw(int radius,int x,int y,char* data,int sizex,int sizey);
protected:
	void Draw01(int radius,int x,int y,char* data,int sizex,int sizey);
	void DrawMmx(int radius,int x,int y,char* data,int sizex,int sizey);
};


cOneCirceData::cOneCirceData(int radius,int delta_radius,Vect2i offset_)
{
	center.x=center.y=radius;
	offset=offset_;
	size.x=size.y=radius*2+1;
	int data_size=(size.x*size.y+7)&~7;
	data=new char[data_size];
	memset(data,127,data_size);//test size

	float mul=1.0f/(1<<quality);
	float cx=center.x+offset.x*mul,cy=center.y+offset.y*mul;

	float dr2inv=1.0f/(sqr(radius)-sqr(radius-delta_radius));
	float delta_minus=sqr(radius-delta_radius);

	for(int y=0;y<size.y;y++)
	for(int x=0;x<size.x;x++)
	{
		float r2=sqr(x-cx)+sqr(y-cy);
		float f=(r2-delta_minus)*dr2inv;
		f=clamp(f,0,1);
		f=sqrtf(f);
		data[x+y*size.x]=round(f*255)-128;
	}
}

cCircleDraw::cCircleDraw(int max_radius_,int shift_)
{
	max_radius=max_radius_;
	shift=shift_;
	quality_shift=shift-cOneCirceData::quality;

	int qmul=cOneCirceData::quality_mul;
	circles.resize((max_radius+1)*qmul*qmul);
	for(int r=0;r<=max_radius;r++)
	{
		for(int y=0;y<qmul;y++)
		for(int x=0;x<qmul;x++)
		{
			int offset=x+y*qmul+r*qmul*qmul;
			if(r==0)
			{
				circles[offset]=0;
				continue;
			}

			int delta_radius=min(r,cFogOfWar::fade_delta_radius>>cFogOfWar::shift);
			circles[offset]=new cOneCirceData(r,delta_radius,Vect2i(x,y));
		}
	}
}

void cCircleDraw::Draw01(int radius,int x,int y,char* data,int sizex,int sizey)
{
	radius>>=shift;
	if(radius==0)return;
	x-=(cFogOfWar::step>>1);y-=(cFogOfWar::step>>1);
	cOneCirceData* c=Get(radius,x,y);
	int xbase=(x>>shift)-c->center.x,ybase=(y>>shift)-c->center.y;
	int xmin=xbase,ymin=ybase;
	int xmax=xmin+c->size.x;
	int ymax=ymin+c->size.y;
	xmin=max(xmin,0);
	xmax=min(xmax,sizex);
	ymin=max(ymin,0);
	ymax=min(ymax,sizey);

	for(int y=ymin;y<ymax;y++)
	{
		char* out=data+xmin+y*sizex;
		int xx=xmin-xbase;
		int yy=y-ybase;
		char* in=c->data+(xx+yy*c->size.x);
		for(int x=xmin;x<xmax;x++,out++,in++)
		{
			*out=min(*in,*out);
		}
	}
}
#pragma warning(disable:4799)
void cCircleDraw::DrawMmx(int radius,int x,int y,char* data,int sizex,int sizey)
{
	radius>>=shift;
	if(radius==0)return;
	x-=(cFogOfWar::step>>1);y-=(cFogOfWar::step>>1);
	cOneCirceData* c=Get(radius,x,y);
	int xbase=(x>>shift)-c->center.x,ybase=(y>>shift)-c->center.y;
	int xmin=xbase,ymin=ybase;
	int xmax=xmin+c->size.x;
	int ymax=ymin+c->size.y;
	xmin=max(xmin,0);
	xmax=min(xmax,sizex);
	ymin=max(ymin,0);
	ymax=min(ymax,sizey);

	static LONGLONG mask[8]=
	{
		0xFFFFFFFFFFFFFFFFu,//0
		0x00000000000000FFu,//1
		0x000000000000FFFFu,//2
		0x0000000000FFFFFFu,//3
		0x00000000FFFFFFFFu,//4
		0x000000FFFFFFFFFFu,//5
		0x0000FFFFFFFFFFFFu,//6
		0x00FFFFFFFFFFFFFFu,//7
	};


	int dx_mask=(xmax-xmin)&7;
	__m64 right_mask;
	right_mask.m64_i64=mask[dx_mask];
	xmax-=8;

	for(int y=ymin;y<ymax;y++)
	{
		__m64* out=(__m64*)(data+xmin+y*sizex);
		int xx=xmin-xbase;
		int yy=y-ybase;
		__m64* in=(__m64*)(c->data+(xx+yy*c->size.x));
		for(int x=xmin;x<xmax;x+=8,out++,in++)
		{
			__m64 out_mmx=_mm_cmpgt_pi8(*in,*out);
			__m64 and_in=_mm_andnot_si64(out_mmx,*in);
			__m64 and_out=_mm_and_si64(out_mmx,*out);
			*out=_mm_or_si64(and_in,and_out);
		}

		__m64 out_mmx=_mm_cmpgt_pi8(*out,*in);
		out_mmx=_mm_and_si64(out_mmx,right_mask);
		__m64 and_in=_mm_and_si64(out_mmx,*in);
		__m64 and_out=_mm_andnot_si64(out_mmx,*out);
		*out=_mm_or_si64(and_in,and_out);
	}
}

void FogOfWarMap::calcSummaryMap()
{
/*
	int inv_scout_alpha=255-fogOfWar_->GetScoutAreaAlpha();
	for(int y=0;y<size.y;y++)
	{
		char* tile=tilemap+y*size.x;
		char* scout=scoutmap+y*size.x;
		char* summary=summarymap+y*size.x;
		for(int x=0;x<size.x;x++,tile++,scout++,summary++)
		{
			int c=127-*scout;
			c=(c*inv_scout_alpha)>>8;
			c=127-c;
			*summary=min(c,*tile);
		}
	}
/*/
	__m64 m127,inv_alpha,zero,shift_count;
	int i;
	for(i=0;i<8;i++)
	{
		m127.m64_u8[i]=127;
		zero.m64_u8[i]=0;
	}
	for(i=0;i<4;i++)
	{
		inv_alpha.m64_u16[i]=255-fogOfWar_->GetScoutAreaAlpha();
	}
	shift_count.m64_u64=8;

	for(int y=0;y<size.y;y++)
	{
		__m64* tile=(__m64*)(tilemap+y*size.x);
		__m64* scout=(__m64*)(scoutmap+y*size.x);
		__m64* summary=(__m64*)(summarymap+y*size.x);
		for(int x=0;x<size.x;x+=8,tile++,scout++,summary++)
		{
			__m64 sub127=_mm_sub_pi8(m127,*scout);//127-*scout;

			__m64 hi=_mm_unpackhi_pi8 (sub127,zero);
			__m64 lo=_mm_unpacklo_pi8 (sub127,zero);
			hi=_mm_mullo_pi16(hi,inv_alpha);
			lo=_mm_mullo_pi16(lo,inv_alpha);
			lo=_mm_sra_pi16 (lo,shift_count);
			hi=_mm_sra_pi16 (hi,shift_count);
			sub127=_mm_packs_pi16 (lo,hi);

			sub127=_mm_sub_pi8(m127,sub127);//127-c;
			__m64 out_mmx=_mm_cmpgt_pi8(sub127,*tile);
			__m64 and_in=_mm_andnot_si64(out_mmx,sub127);
			__m64 and_out=_mm_and_si64(out_mmx,*tile);
			*summary=_mm_or_si64(and_in,and_out);
		}
	}
/**/

	for(int y=0;y<size.y;y++)
	{
		summarymap[y*size.x]=127;
		summarymap[y*size.x+size.x-1]=127;
	}

	for(int x=0;x<size.x;x++)
	{
		summarymap[x]=127;
		summarymap[x+(size.y-1)*size.x]=127;
	}

//	log_var_crc(summarymap, size.x*size.y);

}

void FogOfWarMap::calcScoutMap()
{
	//Не забыть про случай, когда scout_area_alpha==0, и разведанное тождественно равно видимому.
/*
	for(int y=0;y<size.y;y++)
	{
		char* tile=tilemap+y*size.x;
		char* scout=scoutmap+y*size.x;
		for(int x=0;x<size.x;x++,tile++,scout++)
		{
			*scout=min(*tile,*scout);
		}
	}
*/
	for(int y=0;y<size.y;y++)
	{
		__m64* tile=(__m64*)(tilemap+y*size.x);
		__m64* scout=(__m64*)(scoutmap+y*size.x);
		for(int x=0;x<size.x;x+=8,tile++,scout++)
		{
			__m64 out_mmx=_mm_cmpgt_pi8(*scout,*tile);
			__m64 and_in=_mm_andnot_si64(out_mmx,*scout);
			__m64 and_out=_mm_and_si64(out_mmx,*tile);
			*scout=_mm_or_si64(and_in,and_out);
		}
	}

//	log_var_crc(tilemap, size.x*size.y);
//	log_var_crc(scoutmap, size.x*size.y);
}
#pragma warning(default:4799)

const char* FogOfWarMap::lockTilemap() const
{
	lock_.lock();
	return tilemap;
}

void FogOfWarMap::unlockTilemap() const
{
	lock_.unlock();
}

const char* FogOfWarMap::lockScoutmap() const
{
	lock_.lock();
	return scoutmap;
}

void FogOfWarMap::unlockScoutmap() const
{
	lock_.unlock();
}


const char* FogOfWarMap::lockSummarymap() const
{
	// @dilesoft
	try {
		lock_.lock();
	} catch (...) {
	};
	return summarymap;
}

void FogOfWarMap::unlockSummarymap() const
{
	lock_.unlock();
}

void cCircleDraw::Draw(int radius,int x,int y,char* data,int sizex,int sizey)
{
	//DrawUnoptimized(radius,x,y,data,sizex,sizey);//33x512 = 0.25 ms
	//Draw01(radius,x,y,data,sizex,sizey);//33x512 =  0.20 ms
	DrawMmx(radius,x,y,data,sizex,sizey);//33x512 =  0.05 ms
}

//////////////////////cFogOfWar////////////////////////
cFogOfWar::cFogOfWar(TerraInterface* terra_)
:cBaseGraphObject(0)
{
	terra=NULL;
    selected_map=NULL;
	pTextureTilemap=NULL;
	fog_color.set(128,128,128,240);
	invAlpha = 255.f/240;
	scout_area_alpha=220;
	circleDraw = new cCircleDraw(maxSightRadius>>shift, cFogOfWar::shift);
	fogMinimapAlpha_ = 255;
	show_tilemap=true;

	Init(terra_);
}

cFogOfWar::~cFogOfWar()
{
	delete circleDraw;
	RELEASE(pTextureTilemap);
}

void cFogOfWar::Init(TerraInterface* terra_)
{
	terra = terra_;
	size.x = terra->SizeX() >> shift;
	size.y = terra->SizeY() >> shift;

	pTextureTilemap=GetTexLibrary()->CreateAlphaTexture(size.x,size.y,NULL,true);
	//UpdateTexture();

	int pitch;
	int bpp=pTextureTilemap->GetBitsPerPixel();
	xassert(bpp==8);

	BYTE* data=pTextureTilemap->LockTexture(pitch);
	for(int y=0;y<size.y;y++)
	{
		BYTE* p=data+y*pitch;
		memset(p,0,size.x);
	}
	pTextureTilemap->UnlockTexture();

}

void cFogOfWar::AnimateLogic()
{
	start_timer_auto();
	xassert(selected_map != 0);
}

void cFogOfWar::PreDraw(cCamera *pCamera)
{
	if(GetAttribute(ATTRUNKOBJ_IGNORE))
		return;
//	pCamera->Attach(SCENENODE_OBJECTSPECIAL,this);
}

void cFogOfWar::Draw(cCamera *pCamera)
{
	cD3DRender* rd=gb_RenderDevice3D;

	DWORD old_colorwrite=rd->GetRenderState(D3DRS_COLORWRITEENABLE);
	rd->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALPHA);
	int dx=terra->SizeX(),dy=terra->SizeY();
	sColor4c diffuse(255,255,255);

	rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,pTextureTilemap);
	rd->SetBlendStateAlphaRef(ALPHA_NONE);
	rd->psFont->Select();

//	rd->SaveStates();

	cQuadBuffer<sVertexXYZDT1>* quad=rd->GetQuadBufferXYZDT1();
	quad->BeginDraw();
	sVertexXYZDT1 *v=quad->Get();
	v[0].pos.x=0; v[0].pos.y=0; v[0].pos.z=0; v[0].u1()=0; v[0].v1()=0; v[0].diffuse=diffuse;
	v[1].pos.x=0; v[1].pos.y=dy; v[1].pos.z=0; v[1].u1()=0; v[1].v1()=1; v[1].diffuse=diffuse;
	v[2].pos.x=dx; v[2].pos.y=0; v[2].pos.z=0; v[2].u1()=1; v[2].v1()=0; v[2].diffuse=diffuse;
	v[3].pos.x=dx; v[3].pos.y=dy; v[3].pos.z=0; v[3].u1()=1; v[3].v1()=1; v[3].diffuse=diffuse;
	
	quad->EndDraw();

	rd->SetRenderState(D3DRS_COLORWRITEENABLE,old_colorwrite);
}

void cFogOfWar::Animate(float dt)
{
	UpdateTexture();
}

void cFogOfWar::UpdateTexture()
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	int pitch;
	int bpp=pTextureTilemap->GetBitsPerPixel();
	xassert(bpp==8);

	// @dilesoft
	if (!selected_map) {
		return;
	}

	if(show_tilemap)
	{
		const char* tilemap = selected_map->lockSummarymap();
		BYTE* data=pTextureTilemap->LockTexture(pitch);
		for(int y=0;y<size.y;y++)
		{
			BYTE* p=data+y*pitch;
			for(int x=0;x<size.x;x++,p++)
			{
				char h=tilemap[x+y*size.x];
				*p=(int(h+128)*fog_color.a)>>8;
			}
		}
		selected_map->unlockSummarymap();
		pTextureTilemap->UnlockTexture();
	}else
	{
/*
		const char* tilemap = selected_map->lockScoutmap();
		BYTE* data=pTextureTilemap->LockTexture(pitch);
		for(int y=0;y<size.y;y++)
		{
			BYTE* p=data+y*pitch;
			for(int x=0;x<size.x;x++,p++)
			{
				char h=tilemap[x+y*size.x];
				*p=(h!=127)?0:200;//Полуразведанное
			}
		}
		selected_map->unlockScoutmap();
		pTextureTilemap->UnlockTexture();
/*/
		const char* tilemap = selected_map->lockTilemap();
		BYTE* data=pTextureTilemap->LockTexture(pitch);
		for(int y=0;y<size.y;y++)
		{
			BYTE* p=data+y*pitch;
			for(int x=0;x<size.x;x++,p++)
			{
				char h=tilemap[x+y*size.x];
				*p=(h<127)?0:220;//Полностью разведанное
			}
		}
		selected_map->unlockTilemap();
		pTextureTilemap->UnlockTexture();
/**/
	}
}

void cFogOfWar::SelectMap(const FogOfWarMap* map)
{
    selected_map=map;
}

FogOfWarMap* cFogOfWar::CreateMap()
{
    return new FogOfWarMap(this, size);
}

//////////////////////////////////////////////////////////////////////////////

FogOfWarMap::FogOfWarMap(cFogOfWar *const fogOfWar, const Vect2i& size)
: tilemap(0)
, scoutmap(0)
, size(size)
, fogOfWar_(fogOfWar)
{
	xassert(size.x%8==0 && size.y%8==0);
	int size_array=(size.x*size.y+8);/*8-для MMX*/
	size_array=((size_array>>4)+1)<<4;//Округляем до 16 в верхнюю сторону.


	raw_data=new char[size_array*3];//Чтобы локально лежало.
	tilemap = raw_data;;
	scoutmap = raw_data+size_array;
	summarymap = raw_data+2*size_array;
	ClearMap();
}

FogOfWarMap::~FogOfWarMap()
{
	delete raw_data;
}

void FogOfWarMap::ClearMap()
{
	memset(tilemap,127,size.x*size.y);
	memset(scoutmap,127,size.x*size.y);
	memset(summarymap,127,size.x*size.y);
}


FOW_HANDLE FogOfWarMap::createVisible()
{
	int h=objects_.GetIndexFree();
	One& o=objects_[h];
	o.x=o.y=0;
	o.radius=0;
	return h;
}

void FogOfWarMap::deleteVisible(FOW_HANDLE h)
{
	if(h==-1)
		return;
	xassert(h >= 0 && h < objects_.size());
	objects_.SetFree(h);	
}

void FogOfWarMap::moveVisible(FOW_HANDLE h,int x,int y,int radius)
{
	if(h==-1)
		return;
	radius=min(radius,maxSightRadius());
	xassert(h>=0 && h<objects_.size());
	One& obj=objects_[h];
	obj.x=x;
	obj.y=y;
	obj.radius=radius;
}

void FogOfWarMap::moveVisibleOuterRadius(FOW_HANDLE handle, int x, int y, int radius)
{
	int delta_radius=min(radius,cFogOfWar::fade_delta_radius>>cFogOfWar::shift)<<cFogOfWar::shift;
//	delta_radius/=2;
	moveVisible(handle,x,y,radius+delta_radius);//Можно подбирать для лучшего визуального соответствия
}


bool FogOfWarMap::checkFogStateInCircle(Vect2i& centerPosition, int radius) const
{
	int minStepRadius = 15;
	FogOfWarStates status = 0;
	int iStep = radius / minStepRadius + 1;
	for(int i = 0; i <= iStep; i++){  
		int angles = (i == 0) ? 1 : (i + 1) * 2;
		float angleStep = 2.f * M_PI / angles;
		float angle = 0.f;
		while(angle < 2 * M_PI)
		{
			int checkRadius = i * minStepRadius;
			if(checkRadius > radius)
				checkRadius = radius;
			int x = centerPosition.x + checkRadius * cos(angle);
			int y = centerPosition.y + checkRadius * sin(angle);
			status |= getFogState(x, y);
			if(status & FOGST_NONE)
				return true;
			angle += angleStep;
		}
	}

	return false;
}

bool FogOfWarMap::isVisible(const Vect2i& position) const 
{ 
	int x=position.x>>cFogOfWar::shift;
	int y=position.y>>cFogOfWar::shift;
	int ix = x+y*size.x;
	if((UINT)ix<size.x*size.y)
		if (tilemap[ix]<max_opacity)
			return true;
	return false;
}

FogOfWarState FogOfWarMap::getFogState(int x, int y) const
{
	x>>=cFogOfWar::shift;
	y>>=cFogOfWar::shift;
	int ix = x+y*size.x;
	// @dilesoft
	try {
		if((UINT)ix<size.x*size.y)
			if (tilemap[ix]<max_opacity)
				return FOGST_NONE;
			else if (scoutmap[ix]<max_opacity)
				return FOGST_HALF;
		} catch (...) {
	}

	return FOGST_FULL;
}

void FogOfWarMap::drawCircle(int x, int y, int radius)
{
	fogOfWar_->GetCircleDraw()->Draw(radius, x, y, tilemap, size.x, size.y);
}

void FogOfWarMap::quant()
{
	start_timer_auto();
	
	lock_.lock();

	memset(tilemap,127,size.x*size.y);
	for(int h=0;h<objects_.size();h++)
	if(!objects_.IsFree(h))
	{
		One& obj=objects_[h];
		drawCircle(obj.x,obj.y,obj.radius);
//		log_var(obj.x);
//		log_var(obj.y);
//		log_var(obj.radius);
	}

	calcScoutMap();
	calcSummaryMap();
	_mm_empty();

	lock_.unlock();
}

int FogOfWarMap::maxSightRadius()
{
	return cFogOfWar::maxSightRadius;
}

void FogOfWarMap::serialize(Archive& ar)
{
	int mapSize = sizeof(*scoutmap) * size.x * size.y;
	if(ar.isOutput()){
		XBuffer buf(mapSize, 0);
		buf.write(scoutmap, mapSize);
		ar.serialize(buf, "scoutMap", 0);
	}
	else{
		XBuffer buf;
		if(ar.serialize(buf, "scoutMap", 0)){
			if(buf.tell() == mapSize){
				buf.set(0);
				buf.read(scoutmap, mapSize);
			}
			else
				xassert(0);
		}
	}
}
