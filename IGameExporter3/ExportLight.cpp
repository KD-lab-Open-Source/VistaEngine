#include "StdAfx.h"
#include "ExportLight.h"
#include "Interpolate.h"

ExportLight::ExportLight(Saver& saver_,const char* name_)
:saver(saver_),name(name_)
{
	pobject=NULL;
	inode_current=-1;
	LightObj=NULL;
	node=NULL;
}

void ExportLight::Export(IVisLight* pobject_,int inode_current_, ChainsBlock& chains_block)
{
	pobject=pobject_;
	//pobject->InitializeData();
	inode_current=inode_current_;

	//node=pRootExport->GetNode(inode_current);
	//LightObj=(GenLight*)node->GetMaxNode()->EvalWorldState(pRootExport->GetBaseFrameMax()).obj;
	//xassert(LightObj);
	if(pobject->GetType()!=OMNI_LIGHT)
		return;

	saver.push(C3DX_LIGHT);
		saver.push(C3DX_LIGHT_HEAD);
		saver<<inode_current;

		int time=0;
		Point3 color=pobject->GetRGBColor(time);

		Vect3f colorv(color.x,color.y,color.z);
		saver<<colorv;

		float fAttenStart=0,fAttenEnd=0;
		fAttenStart=pobject->GetAttenStart(time);
		fAttenEnd=pobject->GetAttenEnd(time);
		saver<<fAttenStart;
		saver<<fAttenEnd;

		saver.pop();

		for(int i=0;i<pRootExport->animation_data.animation_chain.size();i++)
		{
			AnimationChain& ac=pRootExport->animation_data.animation_chain[i];
			saver.push(C3DX_NODE_CHAIN);
			int interval_size=max(ac.end_frame-ac.begin_frame+1,1);
			SaveColor(ac.begin_frame,interval_size,ac.cycled, chains_block);
			saver.pop();
		}

		const char* tex_name = pobject->GetBitmapName();
		if(tex_name)
		{
			saver.push(C3DX_LIGHT_TEXTURE);
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
			saver.pop();
		}
	saver.pop();
}

void ExportLight::SaveColor(int interval_begin,int interval_size,bool cycled, ChainsBlock& chains_block)
{
	typedef VectTemplate<4> VectColor;
	typedef InterpolatePosition<VectColor> IColor;
	vector<VectColor> color;
	IColor icolor(0);

	for(int icurrent=0;icurrent<interval_size;icurrent++)
	{
		int current_max_time=pRootExport->ToMaxTime(icurrent+interval_begin);
		Point3 c=pobject->GetRGBColor(current_max_time);
		float alpha=pobject->GetIntensity(current_max_time);
		VectColor p;
		p[0]=c.x;
		p[1]=c.y;
		p[2]=c.z;
		p[3]=alpha;
		color.push_back(p);
	}

	icolor.Interpolate(color,0.005f,cycled);
	icolor.interval_size = interval_size;

	/*
	saver.push(C3DX_LIGHT_ANIM_COLOR);
	if(!icolor.Save(saver))
		Msg("Error: Light %s. ITPL_UNKNOWN ROTATION\n",node->GetName());
	saver.pop();
	*/

	int index = chains_block.put(icolor);
	saver.push(C3DX_LIGHT_ANIM_COLOR_INDEX);
	saver << index;
	saver << int(icolor.size());
	saver.pop();
}
