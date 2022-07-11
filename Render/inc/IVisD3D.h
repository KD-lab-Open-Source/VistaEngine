#ifndef __I_VIS_D3D_H_INCLUDED__
#define __I_VIS_D3D_H_INCLUDED__

//Включать этот хидер, если хочется включить "D3DRender.h"

#include <string>
#include <vector>
#include <list>
#include <stack>

using namespace std;

#include <d3d9.h>
#include <d3dx9.h>

#include "..\inc\umath.h"
#include "..\inc\IVisGenericInternal.h"
#include "..\d3d\RenderDevice.h"
#include "..\src\VisError.h"
#include "..\inc\VisGenericDefine.h"
#include "..\inc\RenderMT.h"

#include "..\src\util.h"
#include "..\src\TexLibrary.h"
#include "..\src\Texture.h"
#include "..\src\Texture.inl"
#include "..\src\Material.h"
#include "..\inc\IUnkObj.h"
#include "..\src\Frame.h"
#include "..\src\UnkObj.h"
#include "..\src\cCamera.h"

#include "..\..\Util\Serialization\Saver.h"
#include "..\..\Util\saver_render.h"

#include "..\d3d\D3DRender.h"

#endif
