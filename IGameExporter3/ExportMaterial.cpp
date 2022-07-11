#include "StdAfx.h"
#include "ExportMaterial.h"
#include "Interpolate.h"
#include "..\util\safemath.h"

ExportMaterial::ExportMaterial(Saver& saver)
: saver_(saver)
{
	max_mat=NULL;
	mat=NULL;
}

bool ExportMaterial::Export(Saver& saver, IVisMaterial * mat_, ChainsBlock& chains_block)
{
	mat=mat_;
	const char* mat_name=mat->GetName();
	saver.push(C3DX_MATERIAL);

	//max_mat=mat->GetMaxMaterial();

	{
		saver.push(C3DX_MATERIAL_HEAD);
		saver<<mat_name;
		saver.pop();
	}

	{
		if(!mat->IsStandartType())
		{
			Msg("Error: %s - Неподдерживаемый тип материала\n",mat_name);
			return false;
		}
		saver.push(C3DX_MATERIAL_STATIC);
		float f=0;
		//Point3 p;
		Color p;
		//verify(mat->GetAmbientData()->GetPropertyValue(p));
		p = mat->GetAmbient();
		saver<<p.r<<p.b<<p.b;

		//verify(mat->GetDiffuseData()->GetPropertyValue(p));
		p = mat->GetDiffuse();
		saver<<p.r<<p.g<<p.b;

		//verify(mat->GetSpecularLevelData()->GetPropertyValue(f));
		//verify(mat->GetSpecularData()->GetPropertyValue(p));
		f = mat->GetSpecularLevel();
		p = mat->GetSpecular();
		p.r*=f;
		p.g*=f;
		p.b*=f;
		saver<<p.r<<p.g<<p.b;

		//verify(mat->GetOpacityData()->GetPropertyValue(f));
		f = mat->GetOpacity();
		saver<<f;

		f = mat->GetShininess()*100;
		saver<<f;

		int shading=mat->GetShading();//(StdMat*)max_mat)->GetShading();
		bool no_light=shading==SHADE_PHONG;
		saver<<no_light;
		saver.pop();
	}

	IVisTexmap* texmap_di=0;
	IVisTexmap* texmap_dp=0;
	for(int i=0;i<mat->GetTexmapCount();i++)
	{
		IVisTexmap* tex_map=mat->GetTexmap(i);
		//if(!tex_map->IsEntitySupported())
		//{
		//	Msg("Error: %s - Неподдерживаемый тип TextureMap в материале %s\n", tex_map->GetTextureName(),mat_name);
		//	return false;
		//}
		saver.push(C3DX_MATERIAL_TEXTUREMAP);
		DWORD slot=tex_map->GetSlot();
		saver<<slot;
		const char* tex_name=tex_map->GetBitmapFileName();
		char filename_out[_MAX_PATH]="";
		if(tex_name)
		{
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_EXT];
			_splitpath( tex_name, drive, dir, fname, ext );
			_makepath( filename_out, "", "", fname, ext );
		}
		saver<<filename_out;

		float amount = tex_map->GetAmount(); 
		//float amount = ((StdMat*)mat->GetMtl())->GetTexmapAmt(slot,0);
		saver<<amount;
		int uv_tiling=tex_map->GetTextureTiling();
		saver<<uv_tiling;
		int transparency_filter = mat->GetTransparencyType();
		saver<<transparency_filter;
		if(slot==ID_DI)
			texmap_di=tex_map;
		if(slot==ID_DP)
			texmap_dp=tex_map;
		saver.pop();
	}

	for(int i=0;i<pRootExport->animation_data.animation_chain.size();i++)
	{
		AnimationChain& ac=pRootExport->animation_data.animation_chain[i];
		saver.push(C3DX_NODE_CHAIN);

		{
			saver.push(C3DX_NODE_CHAIN_HEAD);
			saver<<i;
			saver<<ac.name;
			saver.pop();
		}

		int interval_size=max(ac.end_frame-ac.begin_frame+1,1);
		ExportOpacity(saver, ac.begin_frame,interval_size,ac.cycled, chains_block);
		if(texmap_di)
			ExportUV(saver, texmap_di,ac.begin_frame,interval_size,ac.cycled, 0, chains_block);
		if(texmap_dp)
			ExportUV(saver, texmap_dp,ac.begin_frame,interval_size,ac.cycled, 1, chains_block);
		saver.pop();
	}

	saver.pop();
	return true;
}

void ExportMaterial::ExportOpacity(Saver& saver, int interval_begin,int interval_size,bool cycled, ChainsBlock& chains_block)
{
	typedef VectTemplate<1> VectOpacity;
	typedef InterpolatePosition<VectOpacity> IOpacity;
	vector<VectOpacity> opacity;
	IOpacity iopacity(0);

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		int current_max_time=pRootExport->ToMaxTime(icurrent+interval_begin);
		float f=0;
		//verify(mat->GetOpacityData()->GetPropertyValue(f,current_max_time));
		f = mat->GetOpacity(current_max_time);
		VectOpacity p;
		p[0]=f;
		opacity.push_back(p);
	}

	iopacity.Interpolate(opacity,0.005f,cycled);
	iopacity.interval_size = interval_size;

	/*
	saver.push(C3DX_MATERIAL_ANIM_OPACITY);
	if(!iopacity.Save(saver))
	Msg("Error: Material %s. ITPL_UNKNOWN ROTATION\n",mat->GetMaterialName());
	saver.pop();
	*/
	int index = chains_block.put(iopacity);
	saver.push(C3DX_MATERIAL_ANIM_OPACITY_INDEX);
	saver << index;
	saver << int(iopacity.size());
	saver.pop();
}

void TypeCorrectCycleUV(const VectTemplate<6>& example,VectTemplate<6>& target)
{
	const nsub=6;
	VectTemplate<6> rot_prev=example;
	target[4]=uncycle(target[4], example[4],1);
	target[5]=uncycle(target[5], example[5],1);
}


void ExportMaterial::ExportUV(Saver& saver, IVisTexmap* texmap,int interval_begin,int interval_size,bool cycled, int index, ChainsBlock& chains_block)
{
	typedef VectTemplate<6> Mat2D;
	typedef InterpolatePosition<Mat2D> IMat;
	vector<Mat2D> matrix;
	IMat imatrix(TypeCorrectCycleUV);
	bool is_all_identify=true;

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		int current_max_time=pRootExport->ToMaxTime(icurrent+interval_begin);
		Matrix3 uvtrans;
		mat->Validity(current_max_time);
		texmap->GetUVTransform(uvtrans);
		uvtrans.ValidateFlags();
		if(!uvtrans.IsIdentity())
			is_all_identify=false;
		Mat2D p;
		/*
		p[0]= uvtrans.GetRow(0)[0];
		p[1]=-uvtrans.GetRow(0)[1];
		p[2]=-uvtrans.GetRow(1)[0];
		p[3]= uvtrans.GetRow(1)[1];
		p[4]= uvtrans.GetRow(3)[0];
		p[5]=-uvtrans.GetRow(3)[1];
		*/
		p[0]= uvtrans.GetRow(0)[0]; //a00
		p[1]=-uvtrans.GetRow(0)[1]; //a10
		p[2]=-uvtrans.GetRow(1)[0]; //a01
		p[3]= uvtrans.GetRow(1)[1]; //a11
		p[4]= uvtrans.GetRow(3)[0]+uvtrans.GetRow(1)[0]; //a02
		p[5]= 1-uvtrans.GetRow(3)[1]-uvtrans.GetRow(1)[1]; //a12

		matrix.push_back(p);
	}

	if(is_all_identify)
		return;

	imatrix.Interpolate(matrix,0.005f,cycled);
	imatrix.interval_size = interval_size;

	/*
	if(index == 0)
	saver.push(C3DX_MATERIAL_ANIM_UV);
	else
	saver.push(C3DX_MATERIAL_ANIM_UV_DISPLACEMENT);
	if(!imatrix.Save(saver))
	Msg("Error: Material %s. ITPL_UNKNOWN ROTATION\n",mat->GetMaterialName());
	//	Msg("MATERIAL: %s UV=%i\n",mat->GetMaterialName(),imatrix.out_data.size());
	saver.pop();
	*/

	int uvIndex = chains_block.put(imatrix);
	if(index == 0)
		saver.push(C3DX_MATERIAL_ANIM_UV_INDEX);
	else
		saver.push(C3DX_MATERIAL_ANIM_UV_DISPLACEMENT_INDEX);
	saver << uvIndex;
	saver << int(imatrix.size());
	saver.pop();
}
