#include <my_STL.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "XZip.h"

#include "IVisD3D.h"

template<class TYPE>
class DebugType
{
	TYPE t;
	bool debug;
public:
	DebugType(TYPE tt=0):t(tt),debug(false){ }

	inline operator TYPE(){return t;}
	void operator=(TYPE tt){if(!debug)t=tt;}

	void setd(TYPE tt)
	{
		if(tt==-1)
		{
			debug=false;
			return;
		}

		debug=true;
		t=tt;
	}
};

#include "DebugUtil.h"

extern DebugType<int>	Option_MipMapLevel;
extern DebugType<int>	Option_TextureDetailLevel;
extern DebugType<int>	Option_DrawNumberPolygon;
extern DebugType<int>	Option_ShadowSizePower;
extern DebugType<float>	Option_MapLevel;
extern DebugType<int>	Option_ShowRenderTextureDBG;
extern DebugType<int>	Option_ShadowType;
extern DebugType<int>	Option_FavoriteLoadDDS;
extern DebugType<int>	Option_EnableBump;
extern DebugType<int>	Option_ShadowMapSelf4x4;
extern DebugType<float>	Option_ParticleRate;
extern DebugType<int>	Option_TileMapTypeNormal;
extern bool				Option_DetailTexture;
extern DebugType<int>	Option_ShadowTSM;
extern int				Option_FloatZBufferType;
extern DebugType<int>	Option_EnableOcclusionQuery;
extern bool Option_ShowType[SHOW_MAX];
extern DebugType<int>	Option_DprintfLoad;

extern bool Option_IsShadowMap;

extern float Option_HideFactor;
extern float Option_HideRange;
extern bool Option_HideSmoothly;

extern bool Option_UseTextureCache;
extern bool Option_UseMeshCache;
extern bool Option_UseDOF;
extern bool Option_AccessibleZBuffer;
extern DebugType<int>		Option_EnablePreload3dx;

extern class cVisGeneric		*gb_VisGeneric;

#include "..\inc\IncTerra.h"

#ifdef _DEBUG
//#define TEXTURE_NOTFREE
#endif

#include "D3DRender.h"
#include "FileRead.h"

void normalize_path(const char* in_patch,string& out_patch);
int strcmp_null(const char* a,const char* b);
