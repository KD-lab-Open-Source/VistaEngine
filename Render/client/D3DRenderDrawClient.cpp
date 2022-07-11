#include "StdAfxRD.h"

#include "ForceField.h"

class FFDData
{
public:
	int xsize,ysize;
	sPtrIndexBuffer ib;
	sPtrVertexBuffer vb;
	int pagesize;//in vertex
	int pagenumber;
	int curpage;

	int GetNumIndices(){return (2*xsize+2)*ysize;}
	void SetIB(sPolygon* pIndex);
};

void FieldDispatcher::CreateFFDData()
{
	FFDData* p=new FFDData;
	pDrawData=p;

	p->xsize = p->ysize = 1<<(tile_scale-scale);

	cD3DRender* rd=gb_RenderDevice3D;
	rd->CreateIndexBuffer(p->ib,p->GetNumIndices());
	sPolygon* pIndex=rd->LockIndexBuffer(p->ib);
	p->SetIB(pIndex);
	rd->UnlockIndexBuffer(p->ib);

	p->pagesize=(p->xsize+1)*(p->ysize+1);
	p->pagenumber=8;
	p->curpage=0;
	rd->CreateVertexBuffer(p->vb,p->pagesize*p->pagenumber,sVertexXYZDT2::declaration,true);
}

void FieldDispatcher::DeleteFFDData()
{
	pDrawData->vb.Destroy();
	if(pDrawData)
		delete pDrawData;
	pDrawData=NULL;
}

void FFDData::SetIB(sPolygon* pIndex)
{
	#define RIDX(x,y) ((x)+xs*(y))

	int xs = 1 + xsize;
	int ys = ysize;

	WORD *ib=(WORD*)pIndex;

	for (int j = 0; j < ys; j++)
	if (j & 1)
	{
		for (int i = xs-1; i >= 0; i--)
		{
			*ib++ = RIDX(i,j);
			*ib++ = RIDX(i,j+1);
		}
	} else
	{
		for (int i = 0; i < xs; i++)
		{
			*ib++ = RIDX(i,j);
			*ib++ = RIDX(i,j+1);
		}
	}
	#undef RIDX
#ifdef _DEBUG
	int num=ib-(WORD*)pIndex;
	VISASSERT(num==GetNumIndices());
#endif _DEBUG
}

void FieldDispatcher::Draw()
{
	cD3DRender* rd=gb_RenderDevice3D;
	int cull=rd->GetRenderState(D3DRS_CULLMODE);
	rd->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	if(rd->GetDrawNode()->GetCameraPass()==SCENENODE_OBJECT){
		if(tile_global & FieldCluster::TT_OPAQUE)
			Draw(FieldCluster::TT_OPAQUE);
	}
	else{
		if(tile_global & FieldCluster::TT_TRANSPARENT_ADD)
			Draw(FieldCluster::TT_TRANSPARENT_ADD);
		if(tile_global & FieldCluster::TT_TRANSPARENT_MOD)
			Draw(FieldCluster::TT_TRANSPARENT_MOD);
	}
	
	rd->SetRenderState(D3DRS_CULLMODE, cull );
}

void FieldDispatcher::Draw(BYTE transparent)
{
	cD3DRender* rd=gb_RenderDevice3D;

	rd->SetMatrix(D3DTS_WORLD, MatXf::ID);

	VISASSERT( GetTexture(0) && GetTexture(1) );
	Vect3f uv[2];
	Mat3f& mC = rd->GetDrawNode()->GetMatrix().rot();
	uv[0].set(0.5f*mC[0][0],0.5f*mC[0][1],0.5f*mC[0][2]);
	uv[1].set(0.5f*mC[1][0],0.5f*mC[1][1],0.5f*mC[1][2]);
	rd->SetVertexDeclaration(sVertexXYZDT2::declaration);

	FFDData* pfd=pDrawData;
	MTEnter lock(hmap_lock);
	float t=InterpolationFactor();
	float t_=1-t;

	LPDIRECT3DVERTEXBUFFER9 vb=GETIDirect3DVertexBuffer(pfd->vb.ptr->p);
	rd->SetStreamSource(pfd->vb);

	DWORD AlphaTest = rd->GetRenderState(D3DRS_ALPHATESTENABLE);
	DWORD AlphaRef = rd->GetRenderState(D3DRS_ALPHAREF);
	DWORD zwrite = rd->GetRenderState(D3DRS_ZWRITEENABLE);
	
	if(transparent==FieldCluster::TT_TRANSPARENT_ADD){
		rd->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
		rd->SetNoMaterial(ALPHA_ADDBLENDALPHA,MatXf::ID,GetFrame()->GetPhase(),GetTexture(0),
						GetTexture(1),COLOR_ADD);
	}
	else if(transparent==FieldCluster::TT_TRANSPARENT_MOD){
		rd->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
		rd->SetNoMaterial(ALPHA_BLEND,MatXf::ID,GetFrame()->GetPhase(),GetTexture(0),
						GetTexture(1),COLOR_ADD);
	}
	else{
		rd->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
		rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,GetFrame()->GetPhase(),GetTexture(0),
						GetTexture(1),COLOR_ADD);

//		SetRenderState(D3DRS_ALPHAREF,0);
//		SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
	}

	for(int jTile = 0; jTile < tileMapSizeY(); jTile++)
		for(int iTile = 0; iTile < tileMapSizeX(); iTile++){
			if(!(tileMap(iTile, jTile) & transparent)) 
				continue;
			
			if(!rd->GetDrawNode()->TestVisible(Vect3f(t2w(iTile), t2w(jTile),0),
				Vect3f(t2w(iTile + 1), t2w(jTile + 1), 255)))
				continue;
			
			int tile_size = t2m(1);
			int numvertex = (tile_size + 1)*(tile_size + 1);
			float phase = GetFrame()->GetPhase();
			
			sVertexXYZDT2* pv;
			int bytepagesize = pfd->pagesize*pfd->vb.GetVertexSize();
			if(pfd->curpage<pfd->pagenumber){
				RDCALL(vb->Lock(pfd->curpage*bytepagesize,bytepagesize,
					(void**)&pv,D3DLOCK_NOOVERWRITE));
			}
			else{
				RDCALL(vb->Lock(0,0,(void**)&pv,D3DLOCK_DISCARD));
				pfd->curpage = 0;
			}
			
			int flDraw = 0;
			int x_begin = t2m(iTile);
			int y_begin = t2m(jTile);
			for(int y = 0; y <= tile_size; y++)
				for(int x = 0; x <= tile_size; x++){
					int xm = x + x_begin;
					int ym = y + y_begin;
					int xw = m2w(xm);
					int yw = m2w(ym);
					xm = clamp(xm, 1, mapSizeX() - 2);
					ym = clamp(ym, 1, mapSizeY() - 2);
					const FieldDispatcher::Cell& cell = map(xm, ym);
/*
					float zw = cell.height;
/*/
					float zw = interpolateHeight(xm,ym,t,t_);
/**/
					if(cell.specified()){
						xw += cell.specify_delta.x;
						yw += cell.specify_delta.y;
						zw = FieldCluster::ZeroGround;
					}
					sVertexXYZDT2& v = *pv++;
					v.pos.set(xw, yw, zw);
					if(cell.cluster && v.pos.z > FieldCluster::ZeroGround && cell.cluster->GetTT() == transparent){
						v.diffuse.RGBA() = cell.cluster->GetColor(); 
						flDraw = 1;
					}
					else 
						v.diffuse.RGBA() = 0x00000000;
					const Vect3f& n = !cell.specified() ? normal(xm,ym) : cell.specify_normal;
					v.GetTexel().set(n.dot(uv[0]) + 0.5f, n.dot(uv[1]) + 0.5f);
					v.GetTexel2().set((n.y + 1)*0.5f, (n.z + 1)*0.5f - phase);
				}
				vb->Unlock();
				
				if(!flDraw) 
					continue;
				
				rd->SetIndices(pfd->ib);
				RDCALL(rd->lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP,
					pfd->curpage*pfd->pagesize,
					0, numvertex, 0, pfd->GetNumIndices()-2));
				pfd->curpage++;
		}
		
	{
		rd->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_ADD );
		rd->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1 );
		rd->SetTextureBase( 2, NULL );
		rd->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 2 );
		rd->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
	}
	
	rd->SetRenderState(D3DRS_ZWRITEENABLE,zwrite);
	rd->SetRenderState(D3DRS_ALPHATESTENABLE,AlphaTest);
	rd->SetRenderState(D3DRS_ALPHAREF,AlphaRef);
}

