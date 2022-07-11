#include "StdAfx.h"
#include "RootExport.h"
#include "Interpolate.h"
#include "XMath\safemath.h"
#include "FileUtils\FileUtils.h"

bool Exporter::exportMaterial(StaticMaterial& staticMaterial, IVisMaterial* mat)
{
	staticMaterial.name = mat->GetName();

	if(!mat->IsStandartType()){
		Msg("Error: %s - Неподдерживаемый тип материала\n", mat->GetName());
		return false;
	}

	staticMaterial.ambient = convert(mat->GetAmbient());
	staticMaterial.diffuse = convert(mat->GetDiffuse());
	staticMaterial.specular = convert(mat->GetSpecular())*mat->GetSpecularLevel();
	staticMaterial.opacity = mat->GetOpacity();
	staticMaterial.specular_power = max(mat->GetShininess()*100, 1.f);
	staticMaterial.no_light = mat->GetShading() == SHADE_PHONG;
	staticMaterial.transparencyType = (StaticMaterial::TransparencyType)mat->GetTransparencyType();

	//staticMaterial.animation_group_index;

	xassert(!staticMaterial.name.empty());

	IVisTexmap* texmap_di=0;
	IVisTexmap* texmap_dp=0;
	for(int i=0;i<mat->GetTexmapCount();i++){
		IVisTexmap* tex_map = mat->GetTexmap(i);
		string tex_name = extractFileName(tex_map->GetBitmapFileName());
		switch(tex_map->GetSlot()){
		case ID_DI:
			texmap_di = tex_map;
			staticMaterial.tex_diffuse = tex_name;
			staticMaterial.tiling_diffuse = tex_map->GetTextureTiling();
			break;
		case ID_FI:
			staticMaterial.is_skinned = true;
			staticMaterial.tex_skin = tex_name;
			break;
		case ID_BU:
			staticMaterial.tex_bump = tex_name;
			break;
		case ID_RL:
			staticMaterial.tex_reflect = tex_name;
			staticMaterial.reflect_amount = tex_map->GetAmount();
			break;
		case ID_OP:
			staticMaterial.is_opacity_texture = true;
			break;
		case ID_SI:
			staticMaterial.tex_self_illumination = tex_name;
			break;
		case ID_SP:
			staticMaterial.tex_specularmap = tex_name;
			break;
		case ID_DP:
			texmap_dp = tex_map;
			staticMaterial.tex_secondopacity = tex_name;
			break;
		}
	}

	StaticMaterialAnimations& chains = staticMaterial.chains;
	chains.resize(animationChains_.size());
	for(int i=0;i < animationChains_.size();i++){
		StaticAnimationChain& ac = animationChains_[i];

		exportOpacity(chains[i], mat, ac.begin_frame, ac.intervalSize(), ac.cycled);
		
		if(texmap_di)
			exportUV(chains[i].uv, mat, texmap_di, ac.begin_frame, ac.intervalSize(), ac.cycled);
		if(texmap_dp)
			exportUV(chains[i].uv_displacement, mat, texmap_dp, ac.begin_frame, ac.intervalSize(), ac.cycled);
	}
	return true;
}

void Exporter::exportOpacity(StaticMaterialAnimation& chain, IVisMaterial* mat, int interval_begin, int interval_size, bool cycled)
{
	RefinerOpacity iopacity;
	for(int icurrent = 0; icurrent < interval_size;icurrent++){
		int current_max_time = toTime(icurrent + interval_begin);
		VectOpacity value;
		value[0] = mat->GetOpacity(current_max_time);
		iopacity.addValue(value);
	}
	iopacity.refine(0.005f,cycled);
	iopacity.interval_size = interval_size;
	iopacity.export(chain.opacity);
}

struct CycleUVOp
{
	static void cycle(const VectTemplate<6>& example,VectTemplate<6>& target)
	{
		target[4]=uncycle(target[4], example[4],1);
		target[5]=uncycle(target[5], example[5],1);
	}	
};

void Exporter::exportUV(Interpolator3dxUV& uv, IVisMaterial* mat, IVisTexmap* texmap, int interval_begin, int interval_size,bool cycled)
{
	typedef VectTemplate<6> Mat2D;
	typedef Refiner<Mat2D, CycleUVOp> RefinerMat2D;
	RefinerMat2D imatrix;

	bool is_all_identify = true;

	for(int icurrent = 0; icurrent < interval_size; icurrent++){
		int current_max_time = toTime(icurrent+interval_begin);
		Matrix3 uvtrans;
		mat->Validity(current_max_time);
		texmap->GetUVTransform(uvtrans);
		uvtrans.ValidateFlags();
		if(!uvtrans.IsIdentity())
			is_all_identify=false;
		Mat2D p;
		p[0]= uvtrans.GetRow(0)[0]; //a00
		p[1]=-uvtrans.GetRow(0)[1]; //a10
		p[2]=-uvtrans.GetRow(1)[0]; //a01
		p[3]= uvtrans.GetRow(1)[1]; //a11
		p[4]= uvtrans.GetRow(3)[0] + uvtrans.GetRow(1)[0]; //a02
		p[5]= 1 - uvtrans.GetRow(3)[1] - uvtrans.GetRow(1)[1]; //a12

		imatrix.addValue(p);
	}

	if(is_all_identify)
		return;

	imatrix.refine(0.005f,cycled);
	imatrix.interval_size = interval_size;
	imatrix.export(uv);
}
