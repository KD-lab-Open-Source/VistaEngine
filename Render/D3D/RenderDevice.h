#ifndef __RENDER_DEVICE_H_INCLUDED__
#define __RENDER_DEVICE_H_INCLUDED__

#include "..\inc\IRenderDevice.h"
#include "ddraw.h"
#include "..\src\TexLibrary.h"

int RDWriteLog(HRESULT err,char *exp,char *file,int line);
void RDWriteLog(DDSURFACEDESC2 &ddsd);
void RDWriteLog(char *exp,int size=-1);

#define RDCALL(exp)									{ HRESULT hr=exp; if(hr!=DD_OK) RDWriteLog(hr,#exp,__FILE__,__LINE__); VISASSERT(SUCCEEDED(hr)); }
#define RDERR(exp)									{ HRESULT hr=exp; if(hr!=DD_OK) return RDWriteLog(hr,#exp,__FILE__,__LINE__); }

const int POLYGONMAX=1024;

enum eRenderStateCullMode
{
    RENDERSTATE_CULL_NONE	=	1,
    RENDERSTATE_CULL_CW		=	2,
    RENDERSTATE_CULL_CCW	=	3,
    RENDERSTATE_CULL_FORCE	=	0x7fffffff,
};
enum eRenderStateTextureAddress 
{
    TADDRESS_WRAP			= 1,
    TADDRESS_MIRROR			= 2,
    TADDRESS_CLAMP			= 3,
    TADDRESS_BORDER			= 4,
    TADDRESS_FORCE_DWORD	= 0x7fffffff, 
};

class cBaseGraphObject;
class cObjMesh;

void BuildMipMap(int x,int y,int bpp,int bplSrc,void *pSrc,int bplDst,void *pDst,
			 int Blur=0);
void BuildDot3Map(int x,int y,void *pSrc,void *pDst);
void BuildBumpMap(int x,int y,void *pSrc,void *pDst,int fmtBumpMap);

//Величина visMap должна быть TileMap->GetTileNumber().x*visMapDy=TileMap->GetTileNumber().y
void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear);
void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,const Mat3f& direction,sBox6f& box);

#endif
