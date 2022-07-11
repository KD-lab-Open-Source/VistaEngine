// ModelInfo.cpp : implementation file
//

#include "stdafx.h"
//#include "WinVG.h"
#include "ModelInfo.h"

const char* subpath(const char* name,const char* beginpath)
{
	const char* begin=name;
	while(*name && toupper(*name)==toupper(*beginpath))
	{
		name++;
		beginpath++;
	}

	while(name>begin)
	{
		if(*name=='\\' || *name=='/')
		{
			name++;
			break;
		}
		name--;
	}

	return name;
}


void MsgModelInfo(cObject3dx* obj)
{
	//ClearConsole();
	/*
	const char* fullname=obj->GetFileName();
	Msg("%s\n",fullname);
	Msg("MaterialNum=%i\n",obj->GetMaterialNum());
	Msg("NodeNum=%i\n",obj->GetNodeNum());
	for(int ilod=0;ilod<obj->GetStatic()->lods.size();ilod++)
	{
		cStatic3dx::StaticLod& lod=obj->GetStatic()->lods[ilod];
		Msg("Lod%i\n",ilod);
		Msg("  SkinGroupNum=%i\n",lod.bunches.size());
		Msg("  Vertex num=%i\n",lod.vb.GetNumberVertex());
		Msg("  Polygon num=%i\n",lod.ib.GetNumberPolygon());
	}

	if(obj->GetStatic()->debris.vb.IsInit())
	{
		cStatic3dx::StaticLod& lod=obj->GetStatic()->debris;
		Msg("Debris\n");
		Msg("  SkinGroupNum=%i\n",lod.bunches.size());
		Msg("  Vertex num=%i\n",lod.vb.GetNumberVertex());
		Msg("  Polygon num=%i\n",lod.ib.GetNumberPolygon());
	}

	Msg("--------Textures---------\n");
	cStatic3dx* pstatic=obj->GetStatic();
	TextureNames names;
	pstatic->GetTextureNames(names);
	vector<string>::iterator it;
	FOR_EACH(names,it)
	{
		const string& s=*it;
		const char* pc=subpath(s.c_str(),fullname);
		Msg("%s\n",pc);
	}
*/
	//ShowConsole(0);
}
