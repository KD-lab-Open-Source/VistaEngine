#include "StdAfx.h"
#include "Interpolate.h"
#include "FileUtils/FileUtils.h"
#include "RootExport.h"

void StaticLight::export(IVisLight* pobject, int inode_current)
{
	string name = pobject->GetName();
	if(pobject->GetType()!=OMNI_LIGHT || name.find("leaf")!=string::npos)
		return;

	inode = inode_current;
	Vect3f c = convert(pobject->GetRGBColor(0));
	color = Color4f(c.x, c.y, c.z);
	atten_start = pobject->GetAttenStart(0);
	atten_end = pobject->GetAttenEnd(0);

	const StaticAnimationChains& animationChains = exporter->animationChains_;
	chains.resize(animationChains.size());
	for(int i=0;i < animationChains.size();i++){
		const StaticAnimationChain& ac = animationChains[i];
		chains[i].export(pobject, ac.begin_frame, ac.intervalSize(), ac.cycled);
	}

	const char* tex_name = pobject->GetBitmapName();
	if(tex_name)
		texture = extractFileName(tex_name);
}

void StaticLightAnimation::export(IVisLight* pobject, int interval_begin, int interval_size, bool cycled)
{
	RefinerColor icolor;

	for(int icurrent=0;icurrent<interval_size;icurrent++){
		int current_max_time=exporter->toTime(icurrent + interval_begin);
		Point3 c=pobject->GetRGBColor(current_max_time);
		float alpha=pobject->GetIntensity(current_max_time);
		VectColor p;
		p[0]=c.x;
		p[1]=c.y;
		p[2]=c.z;
		p[3]=alpha;
		icolor.addValue(p);
	}

	icolor.refine(0.005f, cycled);
	icolor.interval_size = interval_size;
	icolor.export(color);
}

void StaticLeaf::export(IVisLight* pobject, int inode_current)
{
	string name = pobject->GetName();
	if(pobject->GetType()!=OMNI_LIGHT || name.find("leaf")==string::npos)
		return;
	
	inode = inode_current;
	Vect3f c = convert(pobject->GetRGBColor(0));
	color = Color4f(c.x, c.y, c.z);
	size = pobject->GetAttenEnd(0);

	const char* tex_name = pobject->GetBitmapName();
	if(tex_name)
		texture = extractFileName(tex_name);
}

