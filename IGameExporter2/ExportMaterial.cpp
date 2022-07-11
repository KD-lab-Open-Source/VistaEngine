#include "StdAfx.h"
#include "ExportMaterial.h"
#include "Interpolate.h"

ExportMaterial::ExportMaterial(Saver& saver)
: saver_(saver)
{
	max_mat=NULL;
	mat=NULL;
}

bool ExportMaterial::Export(Saver& saver, IGameMaterial * mat_, ChainsBlock& chains_block)
{
	mat=mat_;
	const char* mat_name=mat->GetMaterialName();
	saver.push(C3DX_MATERIAL);

	max_mat=mat->GetMaxMaterial();

	{
		saver.push(C3DX_MATERIAL_HEAD);
		saver<<mat_name;
		saver.pop();
	}

	{
		if(mat->GetAmbientData()==NULL ||
		   mat->GetDiffuseData()==NULL ||
		   mat->GetSpecularData()==NULL ||
		   mat->GetOpacityData()==NULL ||
		   mat->GetSpecularLevelData()==NULL
		)
		{
			Msg("Error: %s - Неподдерживаемый тип материала\n",mat_name);
			return false;
		}

		saver.push(C3DX_MATERIAL_STATIC);
		float f=0;
		Point3 p;

		verify(mat->GetAmbientData()->GetPropertyValue(p));
		saver<<p.x<<p.y<<p.z;

		verify(mat->GetDiffuseData()->GetPropertyValue(p));
		saver<<p.x<<p.y<<p.z;

		verify(mat->GetSpecularLevelData()->GetPropertyValue(f));
		verify(mat->GetSpecularData()->GetPropertyValue(p));
		p.x*=f;
		p.y*=f;
		p.z*=f;
		saver<<p.x<<p.y<<p.z;

		verify(mat->GetOpacityData()->GetPropertyValue(f));
		saver<<f;

		f=max_mat->GetShininess()*100;
		saver<<f;

		int shading=((StdMat*)max_mat)->GetShading();
		bool no_light=shading==SHADE_PHONG;
		saver<<no_light;
		saver.pop();
	}

	Texmap* texmap_di=0;
	Texmap* texmap_dp=0;
	for(int i=0;i<mat->GetNumberOfTextureMaps();i++)
	{
		IGameTextureMap* tex_map=mat->GetIGameTextureMap(i);
		if(!tex_map->IsEntitySupported())
		{
			Msg("Error: %s - Неподдерживаемый тип TextureMap в материале %s\n", tex_map->GetTextureName(),mat_name);
			return false;
		}
		saver.push(C3DX_MATERIAL_TEXTUREMAP);
			DWORD slot=tex_map->GetStdMapSlot();
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

			StdMat* mb=((StdMat*)max_mat);
			float amount = mb->GetTexmapAmt(slot, 0); 
			saver<<amount;
			Texmap* texmap=tex_map->GetMaxTexmap();
			int uv_tiling=texmap->GetTextureTiling();
			saver<<uv_tiling;
			int transparency_filter = mb->GetTransparencyType();
			saver<<transparency_filter;
			if(slot==ID_DI)
				texmap_di=texmap;
			if(slot==ID_DP)
				texmap_dp=texmap;
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
	IOpacity iopacity;

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		int current_max_time=pRootExport->ToMaxTime(icurrent+interval_begin);
		float f=0;
		verify(mat->GetOpacityData()->GetPropertyValue(f,current_max_time));
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

void ExportMaterial::ExportUV(Saver& saver, Texmap* texmap,int interval_begin,int interval_size,bool cycled, int index, ChainsBlock& chains_block)
{
	typedef VectTemplate<6> Mat2D;
	typedef InterpolatePosition<Mat2D> IMat;
	vector<Mat2D> matrix;
	IMat imatrix;
	bool is_all_identify=true;

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		int current_max_time=pRootExport->ToMaxTime(icurrent+interval_begin);
		Matrix3 uvtrans;
		max_mat->Validity(current_max_time);
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
