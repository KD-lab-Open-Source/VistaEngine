#include "StdAfxRD.h"
#include "ExternalObj.h"

cExternalObj::cExternalObj(ExternalObjFunction func_) 
: cAnimUnkObj(0)
{
	func=func_;
	sort_pass=false;
}
cExternalObj::~cExternalObj()
{
}
void cExternalObj::PreDraw(cCamera *DrawNode)
{
	if(GetAttribute(ATTRUNKOBJ_IGNORE)==0)
	{
		if(GetTexture() && GetTexture()->GetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST))
			DrawNode->Attach(SCENENODE_OBJECTSPECIAL,this);
		else
			DrawNode->Attach(SCENENODE_OBJECT,this);
	}
	
}
void cExternalObj::Draw(cCamera *DrawNode)
{
	extern void terExternalShowCall(int ObjType,void *pointer,int TestSizePerByte);

	DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
	gb_RenderDevice3D->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

	if(GetTexture()->GetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST))
	{
		gb_RenderDevice3D->SetNoMaterial(ALPHA_BLEND,MatXf::ID,GetFrame()->GetPhase(),GetTexture());//FIXME
	}else
	{
		gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,GetFrame()->GetPhase(),GetTexture());//FIXME
	}

	DWORD zwrite=gb_RenderDevice3D->GetRenderState( D3DRS_ZWRITEENABLE);
	if(sort_pass)
		gb_RenderDevice3D->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );

	func();
	
	gb_RenderDevice3D->SetRenderState( D3DRS_ZWRITEENABLE, zwrite );
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,old_cullmode);
}
