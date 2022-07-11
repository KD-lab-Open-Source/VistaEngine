#include "StdAfx.h"

#include "..\terra\terra.h"
#include "..\Render\inc\TerraInterface.inl"

#include "Water.h"
#include "..\Render\inc\IVisD3D.h"
#include "..\Render\shader\shaders.h"
#include "..\Render\src\RenderCubemap.h"
#include "ResourceSelector.h"
#include "WaterGarbage.h"
#include "SkyObject.h"
#include "RangedWrapper.h"

void WatherUpdateMapFunction(const Vect2i& pos1, const Vect2i& pos2,void* data)
{
	((cWater*)data)->UpdateMap(pos1,pos2);
}

float cWater::z_int_to_float;

void cWater::SetDampfK(int k)
{
	minus_scale_p_div_dampf_k=-clamp(k,1,15);
}

class FunctorWaterAndMapZ:public FunctorGetZ
{
	cWater* pWater;
public:
	FunctorWaterAndMapZ(cWater* pWater_)
		:pWater(pWater_)
	{
	}

	float GetZ(float pos_x,float pos_y)
	{
		float out_z;
		int x=round(pos_x),y=round(pos_y);
		if(x>=0 && x<vMap.H_SIZE && y>=0 && y<vMap.V_SIZE)
			out_z=(float)vMap.GetAltWhole(x,y);
		else
			out_z=0;
		out_z=max(out_z,pWater->GetZ(round(pos_x),round(pos_y)));
		return out_z;
	}
};

cWater::cWater()
:cBaseGraphObject(0)
{
	number_polygon=0;
	number_vertex=0;
	zbuffer=NULL;
	z_epsilon=20;
	dt_global=round(0.04f*256);
	grid_size.set(0,0);
	z_int_to_float=1.0f/(1<<z_shift);
	ignoreWaterLevel_ = round(2.f / z_int_to_float);
	rain=-200;
	environment_water=0;
	change_type_statistic=0;
	change_type_statistic_mid=0;
	pSpeedInterface=NULL;

	psShader=new PSWater;
	vsShader=new VSWater;
	if(gb_RenderDevice3D->IsPS20())
	{
		psShader->SetTechnique(WATER_REFLECTION);
		vsShader->SetTechnique(WATER_REFLECTION);
	}else
	{
		psShader->SetTechnique(WATER_EMPTY);
		vsShader->SetTechnique(WATER_EMPTY);
	}
	
	vsShader->Restore();
	psShader->Restore();

	vsShaderLava=new VSWaterLava;
	vsShaderLava->Restore();
	psShaderLava=new PSWaterLava;
	psShaderLava->Restore();

	//pBump=GetTexLibrary()->GetElement("Scripts\\Resource\\balmer\\shader\\waves.dds");
	//pBump1=GetTexLibrary()->GetElement("Scripts\\Resource\\balmer\\shader\\waves1.dds");
	pBump=GetTexLibrary()->GetElement3D("Scripts\\Resource\\balmer\\shader\\waves.dds");
	pBump1=GetTexLibrary()->GetElement3D("Scripts\\Resource\\balmer\\shader\\waves1.dds");
	
	animate_time=0;
	speed_buffer=NULL;

	opacity_gradient.clear();
//	opacity_gradient.GetOrCreateKey(0)->set(1.0f, 1.0f, 1.0f, GetMinPermittedOpacity());
//	opacity_gradient.GetOrCreateKey(0.15f)->set(1.0f, 1.0f, 1.0f, 0.5f);

//	z_epsilon=2<<z_shift;//temp
	//!!!!! Переделать нафиг! Чтобы вместо буфера была линейная аппроксимация.
	//Будет точнее.
	//z(0)->0 z(zend_zero)->0 z(min)->alpha_min z(max)->alpha_max
	// if(z<zend_zero)return 0;
	// if(z<zmin) return (z-zmin)/(zmin-zend_zero)*alpha_min
	// if(z<zmax) return (z-zmax)/(zmax-zmin)*(alpha_max-alpha_min)+alpha_min
	// return alpha_max;
	opacity_gradient.GetOrCreateKey(0)->set(1.0f, 1.0f, 1.0f, 0);
	opacity_gradient.GetOrCreateKey(2.0/256)->set(1.0f, 1.0f, 1.0f, 0.0f/256);
	opacity_gradient.GetOrCreateKey(3.0/256)->set(1.0f, 1.0f, 1.0f, 32.0f/256);
	opacity_gradient.GetOrCreateKey(30.0/256)->set(1.0f, 1.0f, 1.0f, 1);
	opacity_gradient.GetOrCreateKey(1.0f)->set(1.0f, 1.0f, 1.0f, 1.0f);

	//Непонятно почкму вместо последующего кода не вызвали просто SetOpacity(opacity_gradient);
	z_level.clear();
	z_level.resize(5);
	z_level[0].set(1.f,1.f,1.f,0);
	z_level[0].time = 0;
	z_level[1].set(1.f,1.f,1.f,0);
	z_level[1].time = 2.0f/256;
	z_level[2].set(1.f,1.f,1.f,32.f/256);
	z_level[2].time = 4.0f/256;
	z_level[3].set(1.f,1.f,1.f,1);
	z_level[3].time = 40.0f/256;
	z_level[4].set(1.f,1.f,1.f,1);
	z_level[4].time = 1;

	//z_level.GetOrCreateKey(0)->set(1.f,1.f,1.f,0);
	//z_level.GetOrCreateKey(2.0f/256)->set(1.f,1.f,1.f,0);
	//z_level.GetOrCreateKey(4.0f/256)->set(1.f,1.f,1.f,32.f/256);
	//z_level.GetOrCreateKey(40.0f/256)->set(1.f,1.f,1.f,1);
	//z_level.GetOrCreateKey(1)->set(1.f,1.f,1.f,1.f);
	alpha_min=32;
	alpha_max=255;
	zend_zero=2<<cWater::z_shift;
	zmin=4<<cWater::z_shift;
	zmax=40<<cWater::z_shift;
	inv_zmin_zend_zero=1/float(zmin-zend_zero);
	inv_zmax_zmin=1/float(zmax-zmin);

	SetOpacity(z_level);

	change_function=NULL;
	terMapPoint=NULL;
	relativeWaterLevel = 0;
	relativeWaterLeveInt_ = 0;
	minus_scale_p_div_dampf_k=-15;
	pWaterZ=NULL;

	pFunctorZ = new FunctorWaterAndMapZ(this);
	zreflection=0;
	zreflection_interpolated=0;

	pTextureMiniMap = NULL;
	pEnvironmentEarth=NULL;
	//earth_color.set(0.5f,0.5f,0.5f,1);
	earth_color.set(0,0,0,1);
	reflection_color.set(0,0,1,0);
	reflection_brightnes = 0;

	cur_reflect_sky_color.set(0,0.5f,1,1);
	lava_color_.set(0.8f,0.8f,0.4f,1);
	lava_color_ambient_.set(0.5f,0,0,1);
}

cWater::~cWater()
{
	RELEASE(pEnvironmentEarth);
	RELEASE(pWaterZ);
	RELEASE(pBump);
	RELEASE(pBump1);
	terMapPoint->UnRegisterUpdateMap(WatherUpdateMapFunction,this);
	RELEASE(terMapPoint);
	delete[] zbuffer;
	delete[] speed_buffer;
	delete vsShader;
	delete psShader;
	delete vsShaderLava;
	delete psShaderLava;
	RELEASE(pFunctorZ);
	RELEASE(pTextureMiniMap);

}

#define PREC_TRACE_RAY	17
bool cWater::Trace(const Vect3f& begin, const Vect3f& end, Vect3f& intersection)
{
	if ((round(end.x) >> grid_shift) < 0 || (round(end.x) >> grid_shift) > grid_size.x ||
		(round(end.y) >> grid_shift) < 0 || (round(end.y) >> grid_shift) > grid_size.y)
		return false;
	float dx = end.x - begin.x;
	float dy = end.y - begin.y;
	float dz = end.z - begin.z;

	float dxAbs = fabs(dx);
	float dyAbs = fabs(dy);

	if (dxAbs < 1.f && dyAbs < 1.f){
		float z = GetZ(round(begin.x),round(begin.y));
		if (z<begin.z && z>end.z){
			intersection.set(begin.x,begin.y,z);
			return true;
		}
		else
			return false;
	}

	int dx_,dy_,dz_;
	if(dyAbs>dxAbs)
	{
		dx_ = round(dx*(1<<PREC_TRACE_RAY)/dyAbs);
		dy_ = ((dy>=0)?1:-1)<<PREC_TRACE_RAY;
		dz_ = round(dz*(1<<PREC_TRACE_RAY)/dyAbs);
	}else
	{
		dx_ = ((dx>=0)?1:-1)<<PREC_TRACE_RAY;
		dy_ = round(dy*(1<<PREC_TRACE_RAY)/dxAbs);
		dz_ = round(dz*(1<<PREC_TRACE_RAY)/dxAbs);
	}

	int xb = round(begin.x*(1<<PREC_TRACE_RAY));
	int yb = round(begin.y*(1<<PREC_TRACE_RAY));
	int zb = round(begin.z*(1<<PREC_TRACE_RAY));

	int xe = round(end.x*(1<<PREC_TRACE_RAY));
	int ye = round(end.y*(1<<PREC_TRACE_RAY));
	int ze = round(end.z*(1<<PREC_TRACE_RAY));

	int xl = min(xb,xe), xr = max(xb,xe);
	int yl = min(yb,ye), yr = max(yb,ye);

	for(;xb >= xl && xb <= xr && yb >= yl && yb <= yr;
		xb += dx_, yb += dy_, zb += dz_)
	{
		int x = xb>>PREC_TRACE_RAY;
		int y = yb>>PREC_TRACE_RAY;
		if ((x >> grid_shift) < 0 || (x >> grid_shift) >= grid_size.x-1 ||
			(y >> grid_shift) < 0 || (y >> grid_shift) >= grid_size.y-1)
			continue;

		float z = GetZFast(x,y); //GetZ(x,y);
		if (z >= (zb>>PREC_TRACE_RAY))
		{
			intersection.set(xb>>PREC_TRACE_RAY,yb>>PREC_TRACE_RAY, z);
			return true;
		}
	}
	return false;
}

void cWater::PreDraw(cCamera *pCamera)
{
	start_timer_auto();
	if (pEnvironmentEarth)
		pEnvironmentEarth->SetColor(sColor4f(earth_color));
//	pCamera->Attach(SCENENODE_OBJECTFIRST,this);
	pCamera->AttachNoRecursive(SCENENODE_OBJECTSPECIAL,this);

	cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
	if(pReflection)
	{
	//	pReflection->AttachNoRecursive(SCENENODE_OBJECTFIRST,this);
		pReflection->SetZTexture(pWaterZ);
	}

	CalcVisibleLine(pCamera);
	UpdateVB(pCamera);
	IParent->SetZReflection(zreflection_interpolated);
}

void cWater::Draw(cCamera *pCamera)
{
	start_timer_auto();

	cD3DRender* rd=gb_RenderDevice3D;
	if(pCamera->GetAttribute(ATTRCAMERA_REFLECTION))
	{
	//	DrawToZBuffer(pCamera);
		return;
	}

	rd->AddNumPolygonToTilemap();

	int old_zwrite=rd->GetRenderState(RS_ZWRITEENABLE);;
	rd->SetRenderState(RS_ZWRITEENABLE,FALSE);
	DWORD old_fogenable=rd->GetRenderState(D3DRS_FOGENABLE);
//	rd->SetRenderState(D3DRS_FOGENABLE,FALSE);

	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID);
	vsShader->SetSpeedSky(Vect2f(2.0f/size.x,2.0f/size.y),Vect2f(0,fmodf(animate_time*0.01,1.0f)));

	vsShader->EnableZBuffer(gb_VisGeneric->GetFloatZBufferType());
	psShader->EnableZBuffer(gb_VisGeneric->GetFloatZBufferType());

	sColor4f plain_reflection_color;
	plain_reflection_color.mul3(reflection_color,IParent->GetPlainLitColor());
	plain_reflection_color.a=1-reflection_color.a;
	psShader->SetReflectionColor(plain_reflection_color);
	psShader->SetReflectionBrightnes(reflection_brightnes);

	vsShader->Select();
	psShader->Select();

	rd->SetSamplerData(2,sampler_wrap_linear);//Подозрительная строчениция, зачем такие флаги для тумана войны?

	if(vsShader->GetTechnique()==WATER_LINEAR_REFLECTION)
	{
		float time_x=fmodf(animate_time*0.03,1.0f);
		float time_y=fmodf(animate_time*0.02,1.0f);
		float time_x1=fmodf(-animate_time*0.022,1.0f);
		float time_y1=fmodf(-animate_time*0.033,1.0f);
		const float speed_scale=5e-3f;
		const float speed_scale1=7e-3f;
		cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
		vsShader->SetMirrorMatrix(pReflection);
		vsShader->SetSpeed(Vect2f(speed_scale,speed_scale),Vect2f(time_x,time_y));
		vsShader->SetSpeed1(Vect2f(speed_scale1,speed_scale1),Vect2f(time_x1,time_y1));

		rd->SetTexture(0,pBump);
		rd->SetTexture(1,pBump1);
		if(pReflection)
		rd->SetTexture(2,pReflection->GetRenderTarget());

		gb_RenderDevice3D->SetSamplerData( 2,sampler_clamp_anisotropic);
	}else
	if(vsShader->GetTechnique()==WATER_LAVA)
	{
		psShaderLava->SetColors(lava_color_,lava_color_ambient_);
		vsShaderLava->Select();
		psShaderLava->Select();
		vsShaderLava->SetTime(animate_time*0.01f);
	}else
	if(vsShader->GetTechnique()==WATER_REFLECTION)
	{
		float time_x=fmodf(animate_time*0.03,1.0f);
		float time_y=fmodf(animate_time*0.02,1.0f);
		float time_x1=fmodf(-animate_time*0.022,1.0f);
		float time_y1=fmodf(-animate_time*0.033,1.0f);
		const float speed_scale=5e-3f;
		const float speed_scale1=7e-3f;
		vsShader->SetSpeed(Vect2f(speed_scale,speed_scale),Vect2f(time_x,time_y));
		vsShader->SetSpeed1(Vect2f(speed_scale1,speed_scale1),Vect2f(time_x1,time_y1));

		rd->SetTexture(0,pBump);
		rd->SetTexture(1,pBump1);
		rd->SetTexture(2,GetScene()->GetSkyCubemap());

		gb_RenderDevice3D->SetSamplerData( 2,sampler_wrap_linear);
	}else
	if(vsShader->GetTechnique()==WATER_BAD)
	{
	}else
	{
		float time_x=fmodf(animate_time*0.03,1.0f);
		float time_y=fmodf(animate_time*0.02,1.0f);
		float time_x1=fmodf(-animate_time*0.022,1.0f);
		float time_y1=fmodf(-animate_time*0.033,1.0f);
		const float speed_scale=5e-3f;
		const float speed_scale1=7e-3f;
		vsShader->SetSpeed(Vect2f(speed_scale,speed_scale),Vect2f(time_x,time_y));
		vsShader->SetSpeed1(Vect2f(speed_scale1,speed_scale1),Vect2f(time_x1,time_y1));

		rd->SetTexture(0,pBump);
		rd->SetTexture(1,pBump1);
		//sColor4f color;
		//color.mul3(sColor4f(0,0.5f,1,1),IParent->GetPlainLitColor());
		//color.a=1;
		//psShader->SetPS11Color(color);
		psShader->SetPS11Color(cur_reflect_sky_color);
	}
	
//	rd->SetTexture(pBump,0,1);
	gb_RenderDevice3D->SetSamplerData( 0,sampler_wrap_anisotropic);
	gb_RenderDevice3D->SetSamplerData( 1,sampler_wrap_anisotropic);
	gb_RenderDevice3D->SetSamplerData( 3,sampler_clamp_anisotropic);

	DrawPolygons(pCamera);

	rd->SetRenderState(D3DRS_FOGENABLE,old_fogenable);
	rd->SetRenderState(RS_ZWRITEENABLE,old_zwrite);
	rd->SetVertexShader(NULL);
	rd->SetPixelShader(NULL);

	for(int i=0;i<8;i++)
	{
		rd->SetTexture(i,NULL);
	}

	rd->AddNumPolygonToNormal();
}

void cWater::DrawPolygons(cCamera* camera)
{
	cD3DRender* rd=gb_RenderDevice3D;
	int num_draw_polygon=0;
/*
	const int max_polygon=65536;
	for(int i=0;i<vb.size();i++)
	{
		for(int cur_polygon=0;cur_polygon<number_polygon;)
		{
			int cur_num=min(max_polygon,number_polygon-cur_polygon);
			rd->DrawIndexedPrimitive(vb[i],0,number_vertex,ib,cur_polygon,cur_num);
			num_draw_polygon+=cur_num;
			cur_polygon+=cur_num;
		}
	}
/*/

	int tile_polygons=visible_tile_size*visible_tile_size*2;
	int num_tile_x=(grid_size.x-1)/visible_tile_size;
	vector<VISIVLE_LINE>::iterator itv;
	FOR_EACH(visible_tile,itv)
	{
		VISIVLE_LINE& vl=*itv;
		int dy=(grid_size.y-1)/vb.size();
		int idx_vb=(vl.begin_tile_y*visible_tile_size)/dy;
		xassert(idx_vb>=0 && idx_vb<vb.size());
		int offsety=(idx_vb*dy)/visible_tile_size;
		int cur_polygon=tile_polygons*(vl.begin_tile_x+(vl.begin_tile_y-offsety)*num_tile_x);
		int cur_num=tile_polygons*vl.num_tile;
		xassert(cur_polygon>=0 && cur_polygon<65536);
		xassert(cur_num>=0 && cur_num<65536);
		xassert(cur_polygon+cur_num<=65536);
		rd->DrawIndexedPrimitive(vb[idx_vb],0,number_vertex,ib,cur_polygon,cur_num);
		num_draw_polygon+=cur_num;
	}

	if (border.isInit())
	{
		border.Draw(camera);
	}


	if(false)
	{
		char str[64];
		sprintf(str,"water=%i",num_draw_polygon);
		rd->OutText(600,600,str,sColor4f(1,1,1,1));
	}
}

void cWater::DrawToZBuffer(cCamera *pCamera)
{
	cD3DRender* rd=gb_RenderDevice3D;
	rd->AddNumPolygonToTilemap();
	DWORD old_cull=rd->GetRenderState( D3DRS_CULLMODE);
	rd->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	rd->SetWorldMaterial(ALPHA_NONE,MatXf::ID);
	DrawPolygons(pCamera);
	rd->SetRenderState( D3DRS_CULLMODE, old_cull );
	rd->AddNumPolygonToNormal();
}

void cWater::Animate(float dt)
{
	float dtsek=dt*1e-3f;
	animate_time+=dtsek;

	float k=dtsek*0.3f;
//	zreflection_interpolated=zreflection*k+zreflection_interpolated*(1-k);
	zreflection_interpolated=zreflection;
	CalcWaterTextures();
}

void cWater::SetVertexBorder(VType* v,int x, int y) 
{
	OnePoint& c=zbuffer[x+y*grid_size.x];
	float z;
	z = c.SZ();
	if (z<0)
		z=GetEnvironmentWater();
	v->pos.z = z*z_int_to_float;
	sColor4c color;
	CalcColor(color,c.z,255);
	v->diffuse = color;
}

void cWater::CalcVisibleLine(cCamera* pCamera)
{
	visible_tile.clear();
	BYTE* test_grid;
	Vect2i test_size;
	int test_shl;
	pCamera->GetTestGrid(test_grid,test_size,test_shl);
	int dd_x=grid_size.x-1;
	int dd_y=grid_size.y-1;
	int num_tile_x=dd_x/visible_tile_size;
	int num_tile_y=dd_y/visible_tile_size;
	xassert((1<<test_shl)<visible_tile_width);
	int multi_size=visible_tile_width>>test_shl;
	xassert(num_tile_x*multi_size==test_size.x);
	xassert(num_tile_y*multi_size==test_size.y);
	int num_visible=0;
	VISIVLE_LINE cur_line;
	cur_line.num_tile=0;

	for(int y=0;y<num_tile_y;y++)
	{
		for(int x=0;x<num_tile_x;x++)
		{
			bool visible=false;
			for(int ix=0;ix<multi_size;ix++)
			for(int iy=0;iy<multi_size;iy++)
			{
				BYTE c=test_grid[(x*multi_size+ix)+(y*multi_size+iy)*test_size.x];
				if(c)
				{
					visible=true;
					break;
				}
			}

			if(visible)
			{
				if(cur_line.num_tile==0)
				{
					cur_line.begin_tile_x=x;
					cur_line.begin_tile_y=y;
					cur_line.num_tile=1;
				}else
				{
					cur_line.num_tile++;
				}
			}else
			{
				if(cur_line.num_tile>0)
				{
					visible_tile.push_back(cur_line);
					cur_line.num_tile=0;
				}
			}
		}

		if(cur_line.num_tile>0)
		{
			visible_tile.push_back(cur_line);
			cur_line.num_tile=0;
		}
	}
}
/*
void cWater::CalcColor(sColor4c& color,int z,unsigned char opacity_shallow)
{
	BYTE index = z >> z_shift;
	color = opacity_buffer[index];
    color.a = BYTE((int(color.a) * int(opacity_shallow)) >> 8);
}
/*/

void cWater::CalcColor(sColor4c& color,int z,unsigned char opacity_shallow)
{

	color.g=color.b=255;
//	color.a=255;
//	return;

	if(opacity_shallow==0)
	{
		color.r=color.a=0;
		return;
	}
	//z(0)->0 z(zend_zero)->0 z(min)->alpha_min z(max)->alpha_max
	if(z<=zend_zero)
	{
		color.r=color.a=0;
		return;
	}

	if(z<zmin)
	{
		color.a=round((z-zend_zero)*inv_zmin_zend_zero*alpha_min);
		color.r=round((z-zend_zero)*inv_zmin_zend_zero*255);
		return;
	}

	color.r=255;
	if(z<zmax)
	{
		color.a=round((z-zmin)*inv_zmax_zmin*(alpha_max-alpha_min)+alpha_min);
		return;
	}
	
	color.a=alpha_max;

	//BYTE index = z >> z_shift;
	//color = opacity_buffer[index];
	//color.a = BYTE((int(color.a) * int(opacity_shallow)) >> 8);
}
/**/

void cWater::UpdateVB(cCamera *pCamera)
{
	CMatrix matReflectionViewProj;
	matReflectionViewProj.identify();
	if(IParent)
		matReflectionViewProj=IParent->reflection_copy->matViewProj;
	sBox6f bound_box;
//	bound_box.min.set(0.4f,0.4f,0.4f);//-1
//	bound_box.max.set(0.6f,0.6f,0.6f);//+1
	bound_box.min.set(-1,-1,-1);//-1
	bound_box.max.set(+1,+1,+1);//+1

	cD3DRender* rd=gb_RenderDevice3D;
	float mul_texel=1/128.0f;

	int tile_polygons=visible_tile_size*visible_tile_size*2;
	int num_tile_x=(grid_size.x-1)/visible_tile_size;
	vector<VISIVLE_LINE>::iterator itv;

	int cur_idx=0;
	VType* vertex=(VType*)rd->LockVertexBuffer(vb[cur_idx]);
	const float tnt1 = 0.01f;
	const int t1 = round(tnt1/z_int_to_float);

	int int_min_visible_z=INT_MAX;
	FOR_EACH(visible_tile,itv)
	{
		VISIVLE_LINE& vl=*itv;
		int dy=(grid_size.y-1)/vb.size();
		int idx_vb=(vl.begin_tile_y*visible_tile_size)/dy;
		xassert(idx_vb>=0 && idx_vb<vb.size());
		if(idx_vb!=cur_idx)
		{
			rd->UnlockVertexBuffer(vb[cur_idx]);
			cur_idx=idx_vb;
			vertex=(VType*)rd->LockVertexBuffer(vb[cur_idx]);
		}

		int offsety=(idx_vb*dy);

		int ybegin=(vl.begin_tile_y)*visible_tile_size;
		int yend=ybegin+visible_tile_size+1;
		int xbegin=vl.begin_tile_x*visible_tile_size;
		int xend=xbegin+vl.num_tile*visible_tile_size+1;

		for(int y=ybegin;y<yend;y++)
		{
			VType* cur=vertex+(y-offsety)*grid_size.x+xbegin;
			for(int x=xbegin;x<xend;x++,cur++)
			{
				int offset=x+y*grid_size.x;
				OnePoint& c=zbuffer[offset];
				int z;
				BYTE alpha_color;
				if(c.type!=type_filled)
				{
					z=c.underground_z;
					alpha_color=c.ambient_type?255:0;
				}else
				{
					alpha_color=255;
					z=c.SZ();
					int_min_visible_z=min(int_min_visible_z,z);

					if(IParent)
					{
						Vect3f out;
						Vect3f in(x<<grid_shift,y<<grid_shift,z*z_int_to_float);
						if(pCamera->TestVisible(in,0))
						{
							//In first call matReflectionViewProj is bad
							matReflectionViewProj.Convert(in,out);
							bound_box.AddBound(out);
						}
						//Здесь нужно будет вставить ReflectionDrawNode->matViewProj*(x,y,z) - matViewProj взята до умножения на offset_matrix
						//Потом посчитать bound для этого безобразия и скоректировать offset_matrix так, чтобы bound туда влазил
					}
				}
				float wz = z*z_int_to_float;
				sColor4c color;
				CalcColor(color,c.z,alpha_color);
				if(z && c.z<t1)
					wz += tnt1;

				cur->pos.set(x*delta.x,y*delta.y,wz);
				cur->diffuse=color;
			}
		}

	}

	rd->UnlockVertexBuffer(vb[cur_idx]);

	if(int_min_visible_z!=INT_MAX)
		zreflection=int_min_visible_z*z_int_to_float;

	if(IParent)
		IParent->reflection_bound_box=bound_box;
	

	if(border.isInit())
	{
		for(int i=0;i<4; i++)
		{
			VType* v = (VType*)gb_RenderDevice3D->LockVertexBuffer(border.tiles[i].vertexBuffer);
			for(int j=0; j<border.tiles[i].vertexBuffer.GetNumberVertex();j++)
			{
				sColor4c color;
				CalcColor(color,environment_water,255);
				v[j].diffuse = color;
			}
			gb_RenderDevice3D->UnlockVertexBuffer(border.tiles[i].vertexBuffer);
		}
		for(int i=4;i<8;i++)
		{
			VType* v = (VType*)gb_RenderDevice3D->LockVertexBuffer(border.tiles[i].vertexBuffer);
			int j;
			if(i==4||i==7)
			{
				for(j=0; j<grid_size.x; j++)
					SetVertexBorder(v+j,
					(round(v[j].pos.x)>>grid_shift), 
					(round(v[j].pos.y)>>grid_shift));
			}else
			{
				for(j=0; j<grid_size.y; j++)
					SetVertexBorder(v+j,
					(round(v[j].pos.x)>>grid_shift), 
					(round(v[j].pos.y)>>grid_shift));
			}
			for(int k=j; k<border.tiles[i].vertexBuffer.GetNumberVertex();k++)
			{
				sColor4c color;
				CalcColor(color,environment_water,255);
				v[k].diffuse = color;
			}
			gb_RenderDevice3D->UnlockVertexBuffer(border.tiles[i].vertexBuffer);
		}
	}
}

void cWater::Init(cScene* pScene)
{
	pScene->SelectWaterFunctorZ(pFunctorZ);
	
	terMapPoint=pScene->GetTileMap();
	terMapPoint->AddRef();
	TerraInterface* terra=terMapPoint->GetTerra();
	int hsize=terra->SizeX();
	int vsize=terra->SizeY();
	cD3DRender* rd=gb_RenderDevice3D;
	const int max_vertex=65536;
	grid_size.x=(hsize>>grid_shift)+1;
	grid_size.y=(vsize>>grid_shift)+1;
	size.set(hsize,vsize);
	delta.x=1<<grid_shift;
	delta.y=1<<grid_shift;
	inv_delta.x=1.0f/delta.x;
	inv_delta.y=1.0f/delta.y;

	number_vertex=grid_size.x*grid_size.y;
	if(number_vertex>max_vertex)
	{
		vb.resize(2);
		number_vertex=grid_size.x*((grid_size.y-1)/vb.size()+1);
		xassert(number_vertex<max_vertex);
		xassert((grid_size.y-1)%vb.size()==0);
		for(int i=0;i<vb.size();i++)
		{
			rd->CreateVertexBuffer(vb[i],number_vertex,VType::declaration,false);
		}
	}else
	{
		vb.resize(1);
		rd->CreateVertexBuffer(vb[0],number_vertex,VType::declaration,false);
	}
	int dd_x=grid_size.x-1;
	int dd_y=(grid_size.y-1)/vb.size();
	int ddv_x=dd_x+1;
	int ddv_y=dd_y+1;

	number_polygon=2*dd_x*dd_y;
	rd->CreateIndexBuffer(ib,number_polygon);
	sPolygon* ptr=rd->LockIndexBuffer(ib);

/*
	for(int y=0;y<dd_y;y++)
	for(int x=0;x<dd_x;x++)
	{
		int base=x+y*ddv_x;
		ptr++->set(base,base+ddv_x,base+1);
		ptr++->set(base+ddv_x,base+ddv_x+1,base+1);
	}
/*/
	int num_tile_x=dd_x/visible_tile_size;
	int num_tile_y=dd_y/visible_tile_size;
	for(int y=0;y<num_tile_y;y++)
	for(int x=0;x<num_tile_x;x++)
	{
		for(int iy=0;iy<visible_tile_size;iy++)
		for(int ix=0;ix<visible_tile_size;ix++)
		{
			int base=(x*visible_tile_size+ix)+(y*visible_tile_size+iy)*ddv_x;
			ptr++->set(base,base+ddv_x,base+1);
			ptr++->set(base+ddv_x,base+ddv_x+1,base+1);
		}
	}
/**/

	rd->UnlockIndexBuffer(ib);
	InitZBuffer();
	UpdateMap(Vect2i(0,0),Vect2i(grid_size.x<<grid_shift,grid_size.y<<grid_shift));
	UpdateVB(NULL);
	pTextureMiniMap = GetTexLibrary()->CreateAlphaTexture(grid_size.x,grid_size.y,NULL,true);
	UpdateTexture();
}
void cWater::UpdateTexture()
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	int pitch;
	xassert(pTextureMiniMap->GetBitsPerPixel()==8);

	BYTE* data=pTextureMiniMap->LockTexture(pitch);
	const OnePoint* point = zbuffer;
	for(int y=0;y<grid_size.y;y++)
	{
		BYTE* p=data+y*pitch;
		for(int x=0;x<grid_size.x;x++,p++)
		{
			if(point->z > ignoreWaterLevel_){
				xassert(point->z * z_int_to_float <= 255);
				sColor4c color;
				CalcColor(color,point->z,255);
				*p = color.a;
			}
			else
				*p = 0;
			//*p = clamp(point->z*z_int_to_float*5,0,250);
			++point;
		}
	}
	pTextureMiniMap->UnlockTexture();

}

void cWater::ChangeBorderZ()
{
	if (border.isInit())
	{
		for(int i=0; i<8; i++)
		{
			VType* v = (VType*)gb_RenderDevice3D->LockVertexBuffer(border.tiles[i].vertexBuffer);
			int offset=0;
			if(i==4||i==7)
				offset = grid_size.x;
			else
			if(i==5||i==6)
				offset = grid_size.y;
			float z = GetEnvironmentWater();
			for(int j=offset; j<border.tiles[i].vertexBuffer.GetNumberVertex(); j++)
				v[j].pos.z = z;
			gb_RenderDevice3D->UnlockVertexBuffer(border.tiles[i].vertexBuffer);
		}

	}
}

//Заполняет квадратную сетку треугольниками.
template<class VertexType>
int SetBoxBorder(Vect3f base_pos, Vect2i count, Vect2f add, VertexType* vertex, int offset, sPolygon* polygons)
{
	Vect3f pos;
	pos.z = base_pos.z;
	pos.y = base_pos.y;
	int i = offset;
	Vect3f center(vMap.H_SIZE/2, vMap.V_SIZE/2, 0);
	float radius = max(vMap.H_SIZE,vMap.V_SIZE)*2.5f*0.98f;

	int y;
	for(y=0; y<count.y;y++)
	{
		pos.x = base_pos.x;
		for(int x=0; x<count.x;x++)
		{
			vertex[i].pos = pos; 
			Vect3f dp = vertex[i].pos-center; 
			if (dp.norm()>radius)
			{
				dp.Normalize(radius);
				vertex[i].pos = center + dp;
			}

			i++;
			pos.x+=add.x;
		}
		pos.y+=add.y;
	}
	int j = 0;
	for(y=0; y<count.y-1; y++)
	for(int x=0; x<count.x-1; x++)
	{
		int i = x+ y*count.x + offset;
		polygons[j].set(i,i+1,i+count.x); j++;
		polygons[j].set(i+1,i+1+count.x,i+count.x); j++;
	}
	return i;
}

template<class VType>
int SetBoxBorder(Vect3f bp, Vect2i count, Vect2f add, VType* v, int offset, sPolygon* pt, sColor4c color)
{
	Vect3f pos;
	pos.z = bp.z;
	pos.y = bp.y;
	int i = offset;
	Vect3f center(vMap.H_SIZE/2, vMap.V_SIZE/2, 0);
	float radius = max(vMap.H_SIZE,vMap.V_SIZE)*2.5f*0.98f;
	int y;
	for(y=0; y<count.y;y++)
	{
		pos.x = bp.x;
		for(int x=0; x<count.x;x++)
		{
			v[i].pos = pos; 
			Vect3f dp = v[i].pos-center; 
			if (dp.norm()>radius)
			{
				dp.Normalize(radius);
				v[i].pos = center + dp;
			}
			v[i].diffuse = color;

			i++;
			pos.x+=add.x;
		}
		pos.y+=add.y;
	}
	int j = 0;
	for(y=0; y<count.y-1; y++)
	for(int x=0; x<count.x-1; x++)
	{
		int i = x+ y*count.x + offset;
		pt[j].set(i,i+1,i+count.x); j++;
		pt[j].set(i+1,i+1+count.x,i+count.x); j++;
	}
	return i;
}
void cWater::Border::Draw(cCamera* camera)
{
	for(int i=0; i<8; i++)
		if(tiles[i].vertexBuffer.IsInit()&&camera->TestVisible(tiles[i].min,tiles[i].max))
		gb_RenderDevice3D->DrawIndexedPrimitive(tiles[i].vertexBuffer,0,tiles[i].vertexBuffer.GetNumberVertex(),tiles[i].indexBuffer,0,tiles[i].indexBuffer.GetNumberPolygon());
}
void cWater::Border::destroy()
{
	for(int i=0; i<8; i++)
	{
		tiles[i].vertexBuffer.Destroy();
		gb_RenderDevice3D->DeleteIndexBuffer(tiles[i].indexBuffer);
	}
}

void cWater::InitBorder()
{
	if(border.isInit())
	{
		xassert(false);
		border.destroy();
	}
	const int con = border_con;
	const int pr_size = border_pr_size;
	const int far_bord = max(size.x,size.y)*2;
	int x_nn = (size.x+2*con)/pr_size;
	float x_add = float(size.x+2*con)/x_nn;
	int y_nn = (size.y+2*con)/pr_size;
	float y_add = float(size.y+2*con)/y_nn;
	int f_nn  = ((far_bord-con)/pr_size);
	float f_add = float (far_bord-con)/f_nn;
	//0  4  1
	//6  x  5
	//2  7  3

	float z = GetEnvironmentWater();
	sColor4c water_sea(255,255,255,opacity_sea);
	for(int i=0; i<4; i++)
	{
		int vertexSize = (f_nn+1)*(f_nn+1);
		int indexSize = f_nn*f_nn*2;
		Vect2i step;
		Vect3f pos;
		switch(i)
		{
		case 0:
			pos.set(-con,-con,z);
			step.set(-f_add,-f_add);
			break;
		case 1:
			pos.set(size.x+con,-con,z);
			step.set(f_add,-f_add);
			break;
		case 2:
			pos.set(-con,size.y+con,z);
			step.set(-f_add,f_add);
			break;
		case 3:
			pos.set(size.x+con,size.y+con,z);
			step.set(f_add,f_add);
			break;
		}
		gb_RenderDevice3D->CreateVertexBuffer(border.tiles[i].vertexBuffer,vertexSize,VType::declaration);
		gb_RenderDevice3D->CreateIndexBuffer(border.tiles[i].indexBuffer,indexSize);
		VType* v=(VType*)gb_RenderDevice3D->LockVertexBuffer(border.tiles[i].vertexBuffer);
		sPolygon* pt=gb_RenderDevice3D->LockIndexBuffer(border.tiles[i].indexBuffer);
		SetBoxBorder(pos,Vect2i(f_nn+1,f_nn+1),step,v,0,pt,water_sea);
		border.tiles[i].min = v->pos;
		border.tiles[i].max = (v+vertexSize-1)->pos;
		gb_RenderDevice3D->UnlockVertexBuffer(border.tiles[i].vertexBuffer);
		gb_RenderDevice3D->UnlockIndexBuffer(border.tiles[i].indexBuffer);
	}

	for(int i=4; i<8; i++)
	{
		int vertexSize; 
		int indexSize;
		Vect2i step;
		Vect3f pos;
		Vect2i count;
		int addVertex;
		Vect3f lastPoint;
		switch(i)
		{
		case 4:
			pos.set(-con,-con,z);
			step.set(x_add,-f_add);
			addVertex = grid_size.x;
			count.set(x_nn,f_nn);
			lastPoint.set(size.x,0,z);
			break;
		case 5:
			pos.set(size.x+con,-con,z);
			step.set(f_add,y_add);
			addVertex = grid_size.y;
			count.set(f_nn,y_nn);
			lastPoint.set(size.x,size.y,z);
			break;
		case 6:
			pos.set(-con,-con,z);
			step.set(-f_add,y_add);
			addVertex = grid_size.y;
			count.set(f_nn,y_nn);
			lastPoint.set(0,size.y,z);
			break;
		case 7:
			pos.set(-con,size.y+con,z);
			step.set(x_add,f_add);
			addVertex = grid_size.x;
			count.set(x_nn,f_nn);
			lastPoint.set(size.x,size.y,z);
			break;
		}
		vertexSize = (count.x+1)*(count.y+1)+addVertex;
		indexSize = count.x*count.y*2+(addVertex-1)*2;
		gb_RenderDevice3D->CreateVertexBuffer(border.tiles[i].vertexBuffer,vertexSize,VType::declaration);
		gb_RenderDevice3D->CreateIndexBuffer(border.tiles[i].indexBuffer,indexSize);
		VType* v=(VType*)gb_RenderDevice3D->LockVertexBuffer(border.tiles[i].vertexBuffer);
		sPolygon* pt=gb_RenderDevice3D->LockIndexBuffer(border.tiles[i].indexBuffer);
		int sm = addVertex;
		int offset = 0;
		int prev = 0;
		int j = 0;
		for(int k=0;k<addVertex-1;k++)
		{
			int x = k<<grid_shift;
			int sz=0;
			int tp;
			switch(i)
			{
			case 4:
				v[offset].pos.set(x, 0, z); 
				tp = round(float(x)/x_add);
				break;
			case 5:
				v[offset].pos.set(size.x, x, z); sz= f_nn;
				tp = round(float(x)/y_add);
				break;
			case 6:
				v[offset].pos.set(0, x, z); sz= f_nn;
				tp = round(float(x)/y_add);
				break;
			case 7:
				v[offset].pos.set(x, size.y, z);
				tp = round(float(x)/x_add);
				break;
			}			
			v[offset].diffuse = water_sea; 
			if (tp!=prev)
			{
				pt[j].set(offset, sm+prev,sm+tp+sz); j++;
				prev = tp;
				sm += sz;
			}
			pt[j].set(offset, sm+tp, offset+1); j++;
			offset++;
		}
		v[offset].pos=lastPoint; v[offset].diffuse = water_sea; 
		offset++;
		count.x += 1;
		count.y += 1;
		SetBoxBorder(pos,count,step,v,offset,pt+j,water_sea);
		border.tiles[i].min = v->pos;
		border.tiles[i].max = (v+vertexSize-1)->pos;
		gb_RenderDevice3D->UnlockVertexBuffer(border.tiles[i].vertexBuffer);
		gb_RenderDevice3D->UnlockIndexBuffer(border.tiles[i].indexBuffer);
	}

	RELEASE(pEnvironmentEarth);
	pEnvironmentEarth=new cEnvironmentEarth(NULL);
	GetScene()->AttachObj(pEnvironmentEarth);
}

void cWater::ShowEnvironmentWater(bool show_border)
{
	if(border.isInit()==show_border)
		return;

	if(!show_border)
	{
		border.destroy();
		RELEASE(pEnvironmentEarth);
	}else
		InitBorder();
}

void cWater::InitZBuffer()
{
	delete[] zbuffer;
	zbuffer=new OnePoint[grid_size.x*grid_size.y];
	speed_buffer=new Vect2i[grid_size.x*grid_size.y];
	terMapPoint->RegisterUpdateMap(WatherUpdateMapFunction,this);
	TerraInterface* terra=terMapPoint->GetTerra();
	int maxx=terra->SizeX()-1;
	int maxy=terra->SizeY()-1;
	int x,y;
	for(y=0;y<grid_size.y;y++)
	for(x=0;x<grid_size.x;x++)
	{
		int fx=x*delta.x;
		int fy=y*delta.y;
		fx=min(fx,maxx);
		fy=min(fy,maxy);

		OnePoint& cur=Get(x,y);
		Vect2i& speed=GetSpeed(x,y);
		cur.map_z=round(terra->GetZf(fx,fy)*(1<<z_shift));
		cur.z=0;
		cur.underground_z=0;
		cur.type=type_empty;
		cur.ambient_type=0;
		speed.set(0,0);
	}
}

void cWater::UpdateMap(const Vect2i& pos1, const Vect2i& pos2)
{
	int xmin=pos1.x>>grid_shift;
	int ymin=pos1.y>>grid_shift;
	int xmax=(pos2.x>>grid_shift)+1;
	int ymax=(pos2.y>>grid_shift)+1;
	xmax=min(xmax,grid_size.x-1);
	ymax=min(ymax,grid_size.y-1);

	TerraInterface* terra=terMapPoint->GetTerra();
	int maxx=terra->SizeX()-1;
	int maxy=terra->SizeY()-1;
	int x,y;
	for(y=ymin;y<=ymax;y++)
	for(x=xmin;x<=xmax;x++)
	{
		int fx=x*delta.x;
		int fy=y*delta.y;
		fx=min(fx,maxx);
		fy=min(fy,maxy);

		OnePoint& cur=Get(x,y);
		cur.map_z=round(terra->GetZf(fx,fy)*(1<<z_shift));
		UpdateUndergroundZ(x,y);
	}
}

void cWater::SetWater(int x,int y,float z)
{
	OnePoint& one=Get(x,y);
	one.z=z*(1<<z_shift);
}

void cWater::AddWater(int x,int y,float dz)
{
	OnePoint& one=Get(x,y);
	one.z+=round(dz*(1<<z_shift));
	if (one.z <0)
		one.z = 0;
}

void cWater::SetRainConstant(float dz)
{
	rain=round(dz*(1<<z_shift));
}

float cWater::GetRainConstant() const
{
	return float(rain)/float(1<<z_shift);
}

void cWater::AnimateLogic()
{
	start_timer_auto();
//	Sleep(80);
	check_array.clear();
	int dt=dt_global;

	TerraInterface* terra=terMapPoint->GetTerra();
	int x,y;

	int trigger_speed=INT_MAX;
	if(pSpeedInterface)
		trigger_speed=pSpeedInterface->GetTriggerSpeed();

	int maxx=grid_size.x-1;
	int maxy=grid_size.y-1;
	for(x=0;x<grid_size.x;x++)
	{
		OutsideWater(Get(x,0),GetSpeed(x,0),true);
		OutsideWater(Get(x,maxy),GetSpeed(x,maxy),true);
	}
	for(y=0;y<grid_size.y;y++)
	{
		OutsideWater(Get(0,y),GetSpeed(0,y),false);
		OutsideWater(Get(maxx,y),GetSpeed(maxx,y),false);
	}

	//for(y=0;y<grid_size.y;y++)
	//for(x=0;x<grid_size.x;x++)
	//{
	//	OnePoint& cur=Get(x,y);
	//	cur.speed.x=cur.speed.y=0;
	//}
//all
	int max_dv=0;
	for(y=0;y<grid_size.y;y++)
	{
		for(x=0;x<grid_size.x;x++)
		{
			int offset=GetOffset(x,y);
			OnePoint& cur=zbuffer[offset];
			bool bprev=cur.type==type_empty;
			bool waterLevelPrev = cur.z > relativeWaterLeveInt_;
			Vect2i& cur_speed=speed_buffer[offset];
			bool is_speed=false;
			int dv_x=0,dv_y=0;
			
			int cur_sz=cur.SZ();
			if(x>0)
			{
				int prev_offset=offset-1;
				OnePoint& prev=zbuffer[prev_offset];
				Vect2i& prev_speed=speed_buffer[prev_offset];
				int prev_sz=prev.SZ();
				int dp=prev_sz-cur_sz;
				int cur_v_x=dp*minus_scale_p_div_dampf_k;

				int dv=(cur_v_x*dt)>>8;
				if(prev.type==type_filled)
				{
					dv_x=ABS(dv);
					is_speed=dv_x>trigger_speed;
				}

				if(dv>0)
				{
					if(dv>=cur.z)
						dv=cur.z;
				}else
				{
					if(dv<=-prev.z)
						dv=-prev.z;
				}
				prev.z+=dv;
				cur.z+=-dv;
				prev_speed.x-=dv;
				cur_speed.x=-dv;
			}
			if(y>0)
			{
				int prev_offset=offset-grid_size.x;
				OnePoint& prev=zbuffer[prev_offset];
				Vect2i& prev_speed=speed_buffer[prev_offset];
				int prev_sz=prev.SZ();
				int dp=prev_sz-cur_sz;
				int cur_v_y=dp*minus_scale_p_div_dampf_k;
				int dv=(cur_v_y*dt)>>8;
				if(prev.type==type_filled)
				{ 
					dv_y=ABS(dv);
					is_speed|=dv_y>trigger_speed;
				}
				if(dv>0)
				{
					if(dv>=cur.z)
						dv=cur.z;
				}else
				{
					if(dv<=-prev.z)
						dv=-prev.z;
				}
				prev.z+=dv;
				cur.z+=-dv;
				prev_speed.y-=dv;
				cur_speed.y=-dv;
			}

			if(cur.z+rain>0)
				cur.z+=rain;
			else
				cur.z=0;

			bool bcur=cur.z<z_epsilon;
			if(bprev^bcur)
				check_array.push_back(Vect2i(x,y));

			bool waterLevelCur = cur.z > relativeWaterLeveInt_;
			if(waterLevelPrev^waterLevelCur)
			{
				if(change_function)
					change_function(x,y);
			}

			if(cur.type==type_empty && cur.ambient_type)
				CheckType(x,y);

			if(is_speed && pSpeedInterface)
			if(cur.type==type_filled)
			{
				pSpeedInterface->AddSpeedWater(x,y,dv_x,dv_y);
			}

			if(!(cur.z<z_epsilon))
			{
				max_dv=max(max_dv,dv_x);
				max_dv=max(max_dv,dv_y);
			}
		}
	}
	float average=1e-2f;
	change_type_statistic_mid+=(change_type_statistic-change_type_statistic_mid)*average;
	change_type_statistic=0;

	vector<Vect2i>::iterator it;
	FOR_EACH(check_array,it)
	{
		ChangeType(it->x,it->y,true);
	}
	check_array.clear();

	if(pSpeedInterface)
		pSpeedInterface->ProcessSpeedWater();

//	log_var_crc(zbuffer, sizeof(OnePoint)*grid_size.x*grid_size.y);
//	log_var_crc(speed_buffer, sizeof(Vect2i)*grid_size.x*grid_size.y);
}

float cWater::GetZ(int x,int y) const
{
	int xi=x>>grid_shift;
	int yi=y>>grid_shift;
	if(xi<0 || xi>=grid_size.x-1 ||
		yi<0 || yi>=grid_size.y-1)
		return GetEnvironmentWater();
	float cx=(x&grid_and)/float(1<<grid_shift);
	float cy=(y&grid_and)/float(1<<grid_shift);

	const OnePoint* z00=&zbuffer[xi+yi*grid_size.x];
	const OnePoint* z01=z00+1;
	const OnePoint* z10=z00+grid_size.x;
	const OnePoint* z11=z10+1;

	return bilinear(z00->SZ()*z_int_to_float,z01->SZ()*z_int_to_float,
					z10->SZ()*z_int_to_float,z11->SZ()*z_int_to_float,
					cx,cy);
}

float cWater::GetZFast(int x,int y) const
{
	int xi=x>>grid_shift;
	int yi=y>>grid_shift;
	
	if(xi<0 || xi>=grid_size.x || yi<0 || yi>=grid_size.y)
		return GetEnvironmentWater();
	
	return Get(xi, yi).SZ() * z_int_to_float;
}

float cWater::GetDeepWaterFast(int x,int y) const
{
	int xi=x>>grid_shift;
	int yi=y>>grid_shift;
	
	if(xi<0 || xi>=grid_size.x || yi<0 || yi>=grid_size.y)
		return GetEnvironmentWater();
	
	return Get(xi, yi).z * z_int_to_float;
}

bool cWater::isUnderWater(const Vect3f& pos, float radius) const
{
	int xx=pos.xi()>>GetCoordShift(),yy=pos.yi()>>GetCoordShift();
	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
	{
		//xxassert(false, "выход за пределы карты воды в isUnderWater");
		return GetEnvironmentWater()> pos.z;
	}

	const OnePoint& op = Get(xx,yy);
	if(op.z > relativeWaterLeveInt_ && op.realHeight() > pos.z)
		return true;

	if(radius > FLT_EPS){
		int xL=round(pos.x-radius)>>GetCoordShift();
		int xR=round(pos.x+radius)>>GetCoordShift();
		int yT=round(pos.y-radius)>>GetCoordShift();
		int yD=round(pos.y+radius)>>GetCoordShift();

		xassert(xL >= 0 && xR < grid_size.x && yT >= 0 && yD < grid_size.y);

		for(int y=yT; y<=yD; y++){
			for(int x=xL; x<=xR; x++){
				const cWater::OnePoint& op = Get(x,y);
				if(op.z > relativeWaterLeveInt_ && op.realHeight() > pos.z)
					return true;
			}
		}
	}

	return false;
}

bool cWater::isUnderWater(const Vect3f& pos) const
{
	int xx=pos.xi()>>GetCoordShift(),yy=pos.yi()>>GetCoordShift();
	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
		return GetEnvironmentWater()> pos.z;

	const OnePoint& op = Get(xx,yy);
	return op.z > relativeWaterLeveInt_ && op.realHeight() > pos.z;
}

bool cWater::isFullWater(const Vect3f& pos) const
{
	int xx = pos.xi() >> GetCoordShift();
	int yy = pos.yi() >> GetCoordShift();

	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
		return true;

	if(Get(xx, yy).z > relativeWaterLeveInt_)
		return true;

	return false;
}

bool cWater::isWater(const Vect3f& pos, float radius) const
{
	int xx = pos.xi() >> GetCoordShift();
	int yy = pos.yi() >> GetCoordShift();
	
	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
		return true;

	if(Get(xx, yy).z > ignoreWaterLevel_) 
		return true;

	if(radius > FLT_EPS){
		int xL=round(pos.x-radius)>>GetCoordShift();
		int xR=round(pos.x+radius)>>GetCoordShift();
		int yT=round(pos.y-radius)>>GetCoordShift();
		int yD=round(pos.y+radius)>>GetCoordShift();

		xassert(xL >= 0 && xR < grid_size.x && yT >= 0 && yD < grid_size.y);

		for(int y=yT; y<=yD; y++)
			for(int x=xL; x<=xR; x++)
				if(Get(x,y).z > ignoreWaterLevel_)	
					return true;
	}

	return false;
}

float cWater::waterDeep(const Vect3f& pos, float radius) const
{
	int xx = pos.xi() >> GetCoordShift();
	int yy = pos.yi() >> GetCoordShift();

	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
		return GetEnvironmentWater();

	float minRelativeZ(GetRelativeZ(xx, yy));

	if(radius > FLT_EPS){
		int xL=round(pos.x-radius)>>GetCoordShift();
		int xR=round(pos.x+radius)>>GetCoordShift();
		int yT=round(pos.y-radius)>>GetCoordShift();
		int yD=round(pos.y+radius)>>GetCoordShift();

		xassert(xL >= 0 && xR < grid_size.x && yT >= 0 && yD < grid_size.y);

		for(int y=yT; y<=yD; y++)
			for(int x=xL; x<=xR; x++){
				float tempRelativeZ(GetRelativeZ(x,y));
				if(tempRelativeZ < minRelativeZ)	
					minRelativeZ = tempRelativeZ;
			}
	}

	return minRelativeZ;
}

Vect2f cWater::GetVelocity(int x,int y)
{
	int xi=x>>grid_shift;
	int yi=y>>grid_shift;
	float cx=(x&grid_and)/float(1<<grid_shift);
	float cy=(y&grid_and)/float(1<<grid_shift);

	Vect2f v00,v01,v10,v11;
	GetVelocity(v00,xi,yi);
	GetVelocity(v01,xi+1,yi);
	GetVelocity(v10,xi,yi+1);
	GetVelocity(v11,xi+1,yi+1);
	Vect2f vel;
	vel.x=bilinear(v00.x,v01.x,v10.x,v11.x,cx,cy);
	vel.y=bilinear(v00.y,v01.y,v10.y,v11.y,cx,cy);
	return vel;
}


void cWater::SetEnvironmentWater(float z)
{
	environment_water=round(z*(1<<z_shift));
	ChangeBorderZ();
}

float cWater::GetEnvironmentWater() const 
{
	return environment_water*z_int_to_float;
}

void cWater::OutsideWater(OnePoint& cur,Vect2i& cur_speed,bool isx)
{
	int dp=environment_water-cur.SZ();
	int cur_v_x=(dp*minus_scale_p_div_dampf_k)>>8;

	int dv=(cur_v_x*dt_global);
	if(dv>0)
	{
		if(dv>=cur.z)
			dv=cur.z;
	}else
	{
//		if(dv<=-prev.z)
//			dv=-prev.z;
	}
//	prev.z+=dv;
	cur.z+=-dv;
	if(isx)
	{
		cur_speed.x=-dv;
		cur_speed.y=0;
	}else
	{
		cur_speed.x=0;
		cur_speed.y=-dv;
	}
}

static Vect2i ambient[8]=
{
	Vect2i(-1,-1),
	Vect2i(-1,0),
	Vect2i(-1,+1),

	Vect2i(0,-1),
	Vect2i(0,+1),

	Vect2i(+1,-1),
	Vect2i(+1,0),
	Vect2i(+1,+1),
};

void cWater::ChangeType(int x,int y,bool include)//Переписать нахрен!
{
	change_type_statistic++;
	CheckType(x-1,y-1);
	CheckType(x-1,y);
	CheckType(x-1,y+1);

	CheckType(x,y-1);
	if(include)
		CheckType(x,y);
	CheckType(x,y+1);

	CheckType(x+1,y-1);
	CheckType(x+1,y);
	CheckType(x+1,y+1);
}

/*
  Настраивать alpha_delta в зависимости от минимальной прозрачности.

  Есть 3 причины, почему высота воды может дёргаться.
  1. Изменение cur.type (заполнена/незаполнена) водой.
  2. Изменение cur.ambient_type - заполнены ли соседи водой
  3. Изменение mid_num - сколько соседей заполнены водой.

  cur.underground_z - нужно для того, чтобы вода по краям земли была плоская, 
                      а не взбиралась на горку из-за больших треугольников.

  Попробовать такой вариант - нулевой цвет всегда прозрачный.
  Значит можно без анимации обойтись прозрасности.
*/

void cWater::CheckType(int x,int y)
{
	if(x<0 || x>=grid_size.x)
		return;
	if(y<0 || y>=grid_size.y)
		return;

	OnePoint& cur=Get(x,y);
	Vect2i* pi=ambient;

	BYTE offset=1;
	cur.ambient_type=0;
	for(int i=0;i<8;i++)
	{
		int xx=x+ambient[i].x;
		int yy=y+ambient[i].y;
		if(xx<0 || xx>=grid_size.x)
			continue;
		if(yy<0 || yy>=grid_size.y)
			continue;
		OnePoint& close=Get(xx,yy);
		if(!(close.z<z_epsilon))
		{
			cur.ambient_type|=offset;
		}

		offset=offset<<1;
	}

	if(!(cur.z<z_epsilon))
	{
		cur.type=type_filled;
		return;
	}

	cur.type=type_empty;
	if(cur.ambient_type==0)
		return;
	
	int mid_z=0;
	int mid_num=0;
	for(int i=0;i<8;i++)
	{
		int xx=x+pi[i].x;
		int yy=y+pi[i].y;
		if(xx<0 || xx>=grid_size.x)
			continue;
		if(yy<0 || yy>=grid_size.y)
			continue;
		OnePoint& close=Get(xx,yy);
		if(!(close.z<z_epsilon))
		{
			mid_num++;
			mid_z+=close.SZ();
		}
	}

	xassert(mid_num>0);
	int z=mid_z/mid_num;
	if(true)
	{
		cur.underground_z=z;
	}else
	{
		const int shift=6;
		const int mul=1<<shift;
		const int c=20;
		cur.underground_z=(cur.underground_z*(mul-c)+z*c)>>shift;
	}
}

void cWater::BorderCheck(int x,int y)
{
	Vect2i* pi=ambient;
	OnePoint& cur=Get(x,y);
	if(!(cur.z<z_epsilon))
	{
		xassert(cur.type==type_filled);
		return;
	}

	int mid_z=0;
	int mid_num=0;
	for(int i=0;i<8;i++)
	{
		int xx=x+pi[i].x;
		int yy=y+pi[i].y;
		if(xx<0 || xx>=grid_size.x)
			continue;
		if(yy<0 || yy>=grid_size.y)
			continue;
		OnePoint& close=Get(xx,yy);
		if(!(close.z<z_epsilon))
		{
			mid_num++;
			mid_z+=close.SZ();
		}
	}

	if(mid_num>0)
	{
//		xassert(cur.type==type_border);
	}else
	{
		xassert(cur.type==type_empty);
	}
}

void cWater::GetVelocity(Vect2f& vel,int x,int y)
{
	if(x<=0 || y<=0 || x>=grid_size.x || y>=grid_size.y)
	{
		vel.set(0,0);
		return;
	}

	const float vel_const=4000.0f/(1<<z_shift_one);
	Vect2i& cur_speed=GetSpeed(x,y);
	vel.set(cur_speed.x*vel_const,cur_speed.y*vel_const);
}

Vect3f cWater::GetVelocity(const Vect3f& pos) const
{
	int xi = pos.xi() >> grid_shift;
	int yi = pos.yi() >> grid_shift;

	if(xi <= 0 || yi <= 0 || xi >= grid_size.x || yi >= grid_size.y)
		return Vect3f::ZERO;

	static const float vel_const = 4000.0f / (1<<z_shift_one);
	Vect2i& cur_speed = GetSpeed(xi, yi);
	return Vect3f(cur_speed.x * vel_const, cur_speed.y * vel_const, 0.f);
}

void cWater::SetWaterRect(int x,int y,float z,int size)
{
	x=x>>grid_shift;
	y=y>>grid_shift;
	size>>=grid_shift;
	int xmin=max(x-size,0);
	int ymin=max(y-size,0);
	int xmax=min(x+size,grid_size.x-1);
	int ymax=min(y+size,grid_size.y-1);
	for(int yy=ymin;yy<=ymax;yy++)
		for(int xx=xmin;xx<=xmax;xx++)
		{
			OnePoint& one=Get(xx,yy);
			int zall=z*(1<<z_shift);
			if(zall<one.map_z)
				one.z=0;
			else
				one.z=zall-one.map_z;
		}
}

void cWater::AddWaterRect(int x,int y,float dz,int size)
{
	int new_dz = round(dz*(1<<z_shift));
	x=x>>grid_shift;
	y=y>>grid_shift;
	size>>=grid_shift;
	int xmin=max(x-size,0);
	int ymin=max(y-size,0);
	int xmax=min(x+size,grid_size.x-1);
	int ymax=min(y+size,grid_size.y-1);

	if(new_dz<0)
	{
		for(int yy=ymin;yy<=ymax;yy++)
		for(int xx=xmin;xx<=xmax;xx++)
		{
			OnePoint& one=Get(xx,yy);
			one.z=max(one.z+new_dz,0);
		}
	}else
	{
		for(int yy=ymin;yy<=ymax;yy++)
		for(int xx=xmin;xx<=xmax;xx++)
		{
			OnePoint& one=Get(xx,yy);
			one.z+=new_dz;
		}
	}
}

void cWater::SetTechnique(WATER_TYPE set)
{
	if((set==cWater::WATER_REFLECTION || set==WATER_LINEAR_REFLECTION) && !gb_RenderDevice3D->IsPS20())
		set=cWater::WATER_EMPTY;

	vsShader->SetTechnique(set);
	psShader->SetTechnique(set);
}

void cWater::serialize(Archive& ar)
{
	int size=grid_size.x*grid_size.y;
	int buf_size=size*sizeof(OnePoint);
	if(ar.isInput()){
		Vect2i grid_size_in;
		grid_size_in.x=grid_size_in.y=0;
		ar.serialize(grid_size_in, "grid_size", 0);

		if(grid_size.x==grid_size_in.x && grid_size.y==grid_size_in.y){
            XBuffer zbuf(1,1);
            if(ar.serialize(zbuf, "zbuf", 0) && zbuf.tell()==buf_size)
                memcpy(zbuffer,zbuf.buffer(),buf_size);
        }
	}
    else{
		ar.serialize(grid_size, "grid_size", 0);
		XBuffer zbuf(zbuffer,buf_size);
		zbuf.set(buf_size);
		ar.serialize(zbuf, "zbuf", 0);
	}

	ar.serialize(rain, "rain", 0);
	ar.serialize(minus_scale_p_div_dampf_k, "minus_scale_p_div_dampf_k", 0);
	ar.serialize(environment_water, "environment_water", 0);
	ar.serialize(reflection_color,"reflection_color1",0);
	ar.serialize(reflection_brightnes,"reflection_brightnes",0);
	ar.serialize(lava_color_,"lava_color_",0);
	ar.serialize(lava_color_ambient_,"lava_color_ambient_",0);
	UpdateMap(Vect2i(0,0),Vect2i(grid_size.x<<grid_shift,grid_size.y<<grid_shift));
}

void cWater::OnePoint::serialize(Archive& ar)
{
	ar.serialize(z, "z", 0);
	ar.serialize(map_z, "map_z", 0);
	ar.serialize(underground_z, "underground_z", 0);
	ar.serialize(type, "type", 0);
	BYTE alpha=255;
	ar.serialize(alpha, "alpha", 0);
}

ShadowingOptions::ShadowingOptions()
{
	user_ambient_factor=0.75f;
	user_ambient_maximal=0.2f;
	user_diffuse_factor=1.0f;
}

ShadowingOptions::ShadowingOptions(float _user_ambient_factor, float _user_ambient_maximal, float _user_diffuse_factor)
: user_ambient_factor(_user_ambient_factor)
, user_ambient_maximal(_user_ambient_maximal)
, user_diffuse_factor (_user_diffuse_factor)
{}

void ShadowingOptions::serialize (Archive& ar)
{
	ar.serialize(RangedWrapperf(user_ambient_factor,0,10), "ambient_factor",   "Ambient умножение");
	ar.serialize(RangedWrapperf(user_ambient_maximal,0,1), "ambient_maximal",  "Ambient максиум");
	ar.serialize(RangedWrapperf(user_diffuse_factor,0,1),  "diffuse_factor",   "Diffuse умножение");
}

void cWater::ClampOpacity(CKeyColor& gradient)
{
/*
	for(int i=0;i<gradient.size();i++)
	{
		sColor4f& c=gradient[i].Val();
		c.a=max(c.a,GetMinPermittedOpacity());
		if(i==0)
		{
//			c.a=0;//Затычка для выставки, потом думать.
		}
	}
	*/
}

void cWater::SetOpacity(const CKeyColor& gradient)
{
	//opacity_gradient = gradient;
	opacity_shallow=gradient.Get(0.0f).a*255;
	opacity_sea=gradient.Get(1.0f).a*255;
	//for(int i = 0; i <= 255; ++i)
	//	opacity_buffer[i] = sColor4c(opacity_gradient.Get(float(i) / 255.0f));
	z_level=gradient;

	if(gradient.size()>1)
	{
		zend_zero=round(gradient[1].time*255)<<z_shift;
	}else
	{
		zend_zero=2<<z_shift;
	}

	if(gradient.size()>2)
	{
		zmin=round(gradient[2].time*255)<<z_shift;
		alpha_min=round(gradient[2].a*255);
	}else
	{
		zmin=4<<z_shift;
		alpha_min=opacity_sea;
	}

	if(gradient.size()>3)
	{
		zmax=round(gradient[3].time*255)<<z_shift;
		alpha_max=round(gradient[3].a*255);
	}else
	{
		zmax=40;
		alpha_max=opacity_sea;
	}

	if(zmin==zend_zero)
		zmin++;
	if(zmin==zmax)
		zmax++;
	float min_dz=1e-3f;
	inv_zmin_zend_zero=1/max(float(zmin-zend_zero),min_dz);
	inv_zmax_zmin=1/max(float(zmax-zmin),min_dz);

	ChangeColor();
}


void cWater::ChangeColor()
{
}

void cWater::UpdateUndergroundZ(int x,int y)
{
	OnePoint& cur=Get(x,y);
	if(cur.type==type_filled || cur.ambient_type)
		return;

	int terra_z=(1<<z_shift);
	int z=cur.map_z;
	for(int i=0;i<8;i++)
	{
		int xx=x+ambient[i].x;
		int yy=y+ambient[i].y;
		if(xx<0 || xx>=grid_size.x)
			continue;
		if(yy<0 || yy>=grid_size.y)
			continue;
		OnePoint& close=Get(xx,yy);
		z=min(z,close.map_z);
	}

	cur.underground_z=z;
}

float cWater::analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int delta = max(radius >> GetCoordShift(), 1);
	int xc = clamp(center.x >> GetCoordShift(), delta, grid_size.x - delta - 1);
	int yc = clamp(center.y >> GetCoordShift(), delta, grid_size.y - delta - 1);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			int z = round(Get(x + xc, y + yc).realHeight());
			Sz += z;
			Sxz += x*z;
			Syz += y*z;
		}
	}

	float t14 = delta*delta;
	float t13 = 2.0f*delta+1.0f;
	float t9 = 2.0f*delta;
	float t8 = 6.0f*t14;
	float N = 4.0f*t14+t9+t13;
	float A = 3.0f*Sxz/delta/(t9+1.0f+t8+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	float B = 3.0f*Syz/delta/(t8+t13+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	normalNonNormalized.set(-A, -B, 4);
	return (float)Sz/N;
}

void cWater::findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax)
{
	zMin = INT_INF;
	zMax = 0;

	int xL = clamp(center.x - radius >> GetCoordShift(), 0, grid_size.x - 1);
	int xR = clamp(center.x + radius >> GetCoordShift(), 0, grid_size.x - 1);
	int yT = clamp(center.y - radius >> GetCoordShift(), 0, grid_size.y - 1);
	int yD = clamp(center.y + radius >> GetCoordShift(), 0, grid_size.y - 1);
	for(int y=yT; y<=yD; y++){
		for(int x=xL; x<=xR; x++){
			int z = Get(x, y).z;
			zMin = min(zMin, z);
			zMax = max(zMax, z);
		}
	}

	zMin = round(zMin*z_int_to_float);
	zMax = round(zMax*z_int_to_float);
}

void cWater::showDebug() const{
	int step = 4;
	for(int x = 0; x < vMap.H_SIZE; x+=step)
		for(int y = 0; y < vMap.V_SIZE; y+=step){
			const OnePoint& q = Get(x >> grid_shift, y >> grid_shift);
			if(q.z > ignoreWaterLevel_)
				show_line(Vect3f(x, y, vMap.getVoxelW(x, y)), Vect3f(x, y, q.realHeight()), q.z > relativeWaterLeveInt_ ? sColor4c(0, 0, 200, 150) : sColor4c(0, 200, 0, 150));
		}
}


void cWater::CalcWaterTextures()
{
	bool is_reflection=IParent->IsReflection();
	if(is_reflection)
	{
		if(!pWaterZ)
		{
			pWaterZ=GetTexLibrary()->CreateTexture(grid_size.x,grid_size.y,SURFMT_A8L8,true);
		}else
		{
			if(grid_size.x!=pWaterZ->GetWidth() || grid_size.y!=pWaterZ->GetHeight())
				xassert(0);
		}
	}

	int pitch_reflection=0;
	BYTE* pDataReflection=NULL;
	if(is_reflection)
		pDataReflection=pWaterZ->LockTexture(pitch_reflection);
	int pitch_minimap;
	xassert(pTextureMiniMap->GetBitsPerPixel()==8);
	BYTE* dataMiniMap=pTextureMiniMap->LockTexture(pitch_minimap);
	sColor4c color;
	for(int y=0;y<grid_size.y;y++)
	{
		WORD* p=(WORD*)(pDataReflection+y*pitch_reflection);
		BYTE* pMiniMap=dataMiniMap+y*pitch_minimap;
		OnePoint* pz=zbuffer+y*grid_size.x;
		for(int x=0;x<grid_size.x;x++,p++,pz++)
		{
			OnePoint& cur=*pz;
			if(pDataReflection)
			{
				int z;
				if(cur.type!=type_filled)
				{
					z=cur.underground_z;
				}else
				{
					z=cur.SZ();
				}

				*p=(z>>(z_shift-8));
			}

			if(cur.z > ignoreWaterLevel_)
			{
				CalcColor(color,cur.z,255);
				*pMiniMap = color.a;
			}
			else
				*pMiniMap = 0;
			pMiniMap++;
		}
	}

	if(pDataReflection)
		pWaterZ->UnlockTexture();
	pTextureMiniMap->UnlockTexture();
}

void cEnvironmentEarth::SetTexture(const char* texture)
{
	RELEASE(Texture);
	if (texture && texture[0])
	{
		Texture = GetTexLibrary()->GetElement3D(texture);
	}

	if (earth_vb.IsInit())
	{
		cD3DRender* rd=gb_RenderDevice3D;
		VType* cur_vertex=(VType*)rd->LockVertexBuffer(earth_vb);
		Vect2f tex_beg(0,0);
		Vect2f tex_size(500,500);
		if(Texture)
		{
			tex_size.x = Texture->GetWidth();
			tex_size.y = Texture->GetHeight();
		}

		for(int i=0;i<size_vb;i++)
		{
			float u = (cur_vertex[i].pos.x - tex_beg.x)/tex_size.x;
			float v = (cur_vertex[i].pos.y - tex_beg.y)/tex_size.y;
			cur_vertex[i].GetTexel().set(u, v);	
			cur_vertex[i].diffuse.set(255,255,255,255);
		}
		rd->UnlockVertexBuffer(earth_vb);
	}
}

class PSEnvironmentEarth:public PSStandart
{
	SHADER_HANDLE tfactor;
public:
	void RestoreShader()
	{
		LoadShader("NoMaterial\\EnvironmentEarth.psl");
	}

	void GetHandle()
	{
		__super::GetHandle();
		GetVariableByName(tfactor,"tfactor");
	}

	void SetColor(sColor4f color)
	{
		SetVector(tfactor,(D3DXVECTOR4*)&color);
	}
};

cEnvironmentEarth::cEnvironmentEarth(const char* texture):cBaseGraphObject(0)
{
	color.set(1,1,1,1);
	psEnvironmentEarth=new PSEnvironmentEarth;
	psEnvironmentEarth->Restore();
//	this->time = time;
	Texture = NULL;
	sur_z = 0;
	cD3DRender* rd=gb_RenderDevice3D;
	Vect2i size((int)vMap.H_SIZE, (int)vMap.V_SIZE);
	const int con = 0;
	const int pr_size = 512;
//	int far_bord = max(size.x,size.y)*2; //sqrtf(sqr(2*size.x) + sqr(2*size.y));
	const int radius = max(size.x,size.y)*2;
	const int far_bord = radius;//sqrtf(sqr(radius)/2);
	int x_nn = (size.x-2*con)/pr_size;
	float x_add = float(size.x-2*con)/x_nn;
	int y_nn = (size.y-2*con)/pr_size;
	float y_add = float(size.y-2*con)/y_nn;
	int f_nn  = ((far_bord+con)/pr_size);
	float f_add = float (far_bord+con)/f_nn;
	size_vb = (x_nn+1)*(f_nn+1)*2 + (y_nn+1)*(f_nn+1)*2 + (f_nn+1)*(f_nn+1)*4;
	size_ib = (x_nn)*(f_nn)*4 + (y_nn)*(f_nn)*4 + (f_nn)*(f_nn)*8;
	rd->CreateVertexBuffer(earth_vb, size_vb,VType::declaration);
	rd->CreateIndexBuffer(earth_ib, size_ib);
	VType* cur_vertex=(VType*)rd->LockVertexBuffer(earth_vb);
	sPolygon* pt=rd->LockIndexBuffer(earth_ib);
	int offset = 0;
	int j = 0;
	float z = sur_z;
	offset = SetBoxBorder(Vect3f(con,con,z), Vect2i(x_nn+1,f_nn+1), Vect2f(x_add,-f_add), cur_vertex, offset, pt+j);
	j+=(x_nn)*(f_nn)*2;
	offset = SetBoxBorder(Vect3f(size.x-con,size.y-con,z), Vect2i(x_nn+1,f_nn+1), Vect2f(-x_add,f_add), cur_vertex, offset, pt+j);
	j+=(x_nn)*(f_nn)*2;

	offset = SetBoxBorder(Vect3f(con, con,z), Vect2i(f_nn+1, y_nn+1), Vect2f(-f_add, y_add), cur_vertex, offset, pt+j);
	j+=(y_nn)*(f_nn)*2;
	offset = SetBoxBorder(Vect3f(size.x-con, size.y-con,z), Vect2i(f_nn+1, y_nn+1), Vect2f(f_add, -y_add), cur_vertex, offset, pt+j);
	j+=(y_nn)*(f_nn)*2;

	offset = SetBoxBorder(Vect3f(-far_bord, con,z), Vect2i(f_nn+1, f_nn+1), Vect2f(f_add, -f_add), cur_vertex, offset, pt+j);
	j+=(f_nn)*(f_nn)*2;
	offset = SetBoxBorder(Vect3f(size.x-con, con,z), Vect2i(f_nn+1, f_nn+1), Vect2f(f_add, -f_add), cur_vertex, offset, pt+j);
	j+=(f_nn)*(f_nn)*2;

	offset = SetBoxBorder(Vect3f(con, size.y-con,z), Vect2i(f_nn+1, f_nn+1), Vect2f(-f_add, f_add), cur_vertex, offset, pt+j);
	j+=(f_nn)*(f_nn)*2;
	offset = SetBoxBorder(Vect3f(size.x+far_bord, size.y-con,z), Vect2i(f_nn+1, f_nn+1), Vect2f(-f_add, f_add), cur_vertex, offset, pt+j);
	j+=(f_nn)*(f_nn)*2;

	xassert(size_vb == offset);
	xassert(size_ib == j);

	rd->UnlockVertexBuffer(earth_vb);
	rd->UnlockIndexBuffer(earth_ib);
	SetTexture(texture);
}

cEnvironmentEarth::~cEnvironmentEarth()
{
	delete psEnvironmentEarth;
	RELEASE(Texture);
}

void cEnvironmentEarth::PreDraw(cCamera *pCamera)
{
	pCamera->Attach(SCENENODE_OBJECTFIRST,this);
}

void cEnvironmentEarth::Draw(cCamera *pCamera)
{
	if(pCamera->GetAttr(ATTRCAMERA_REFLECTION))
		return;

	if (earth_vb.IsInit())
	{
		cD3DRender* rd=gb_RenderDevice3D;
		cScene* pScene=pCamera->GetScene();
		sColor4f tilecolor=pScene->GetTileMap()->GetDiffuse();
		Vect3f dir = pScene->GetSunDirection();
		float a = -dir.dot(Vect3f(0,0,1));
		tilecolor.r=min((tilecolor.r*a+tilecolor.a)*0.5f,1.0f);
		tilecolor.g=min((tilecolor.g*a+tilecolor.a)*0.5f,1.0f);
		tilecolor.b=min((tilecolor.b*a+tilecolor.a)*0.5f,1.0f);
		tilecolor.a=1;

		gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_anisotropic);
		tilecolor*=color;
		rd->SetWorldMaterial(ALPHA_NONE, MatXf::ID, 0, Texture?Texture:rd->GetWhiteTexture());

		psEnvironmentEarth->SetColor(tilecolor);
		psEnvironmentEarth->Select();

		rd->DrawIndexedPrimitive(earth_vb,0,size_vb,earth_ib,0,size_ib);
	}
}

