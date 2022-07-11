#include "StdAfxRd.h"
#include "Static3dx.h"
#include "Node3dx.h"
#include "NParticle.h"
#include "VisError.h"
#include <algorithm>
#include "scene.h"
cTempMesh3dx* FakeCutMesh(cTempMesh3dx* lod0,int lod);
cTempMesh3dx* RealCutMesh(cTempMesh3dx* lod0,int lod);

//Может быть несколько объектов с одним именем, но разными материалами.


void cStatic3dx::BuildProgramLod(vector<cTempMesh3dx*>& temp_mesh)
{
	for(int ivs=0;ivs<visible_sets.size();ivs++)
	{
		cStaticVisibilitySet* pvs=visible_sets[ivs];
		if(pvs->BuildVisibilityGroupsLod())
			continue;
		//добавляем нужные visible_groups и генерируем соответствующие cTempMesh3dx*

		if(pvs->raw_visibility_groups[0]->is_invisible_list)
			break;//Для старых объектов не строим LOD

		int num_visibility_groups=pvs->raw_visibility_groups.size();		
		for(int ilod=0;ilod<cStaticVisibilitySet::num_lod;ilod++)
			pvs->visibility_groups[ilod].resize(num_visibility_groups);
		for(int ivg=0;ivg<num_visibility_groups;ivg++)
		{
			pvs->visibility_groups[0][ivg]=pvs->raw_visibility_groups[ivg];
			pvs->visibility_groups[0][ivg]->lod=0;

			for(int ilod=1;ilod<cStaticVisibilitySet::num_lod;ilod++)
			{
				cStaticVisibilityChainGroup* p=new cStaticVisibilityChainGroup;
				*p=*pvs->visibility_groups[0][ivg];
				pvs->raw_visibility_groups.push_back(p);
				pvs->visibility_groups[ilod][ivg]=p;
				p->lod=ilod;
				for(int ivo=0;ivo<p->temp_visible_object.size();ivo++)
				{
					vector<cTempMesh3dx*> pmesh=FindBuildProgramLod(p->temp_visible_object[ivo].c_str(),temp_mesh,ilod,pvs);
					if(!pmesh.empty())
						p->temp_visible_object[ivo]=pmesh[0]->name;
				}
			}
		}

	}
}

static const char* GetSuffixByLod(int lod)
{
	xassert(lod==1 || lod==2);
	return (lod==1)?"\1lod1":"\1lod2";
}

vector<cTempMesh3dx*> cStatic3dx::FindBuildProgramLod(const char* mesh_name,vector<cTempMesh3dx*>& temp_mesh,int lod,cStaticVisibilitySet* pset)
{
	vector<cTempMesh3dx*> out;
	string lod_name=mesh_name;
	lod_name+=GetSuffixByLod(lod);

	for(int i=0;i<temp_mesh.size();i++)
	{
		cTempMesh3dx* p=temp_mesh[i];
		if(p->name==lod_name)
			out.push_back(p);
	}

	if(out.empty())
	for(int i=0;i<temp_mesh.size();i++)
	{
		cTempMesh3dx* p=temp_mesh[i];
		if(p->name==mesh_name)
		{
			cTempMesh3dx *lod1,*lod2;
			BuildProgramLod(p,lod1,lod2);
			if(lod1)
			{
				lod1->name=string(mesh_name)+GetSuffixByLod(1);
				temp_mesh.push_back(lod1);
				pset->mesh_in_set.push_back(lod1->name);
			}

			if(lod2)
			{
				lod2->name=string(mesh_name)+GetSuffixByLod(2);
				temp_mesh.push_back(lod2);
				pset->mesh_in_set.push_back(lod2->name);
			}

			cTempMesh3dx *cur_out=lod==1?lod1:lod2;
			if(cur_out)
				out.push_back(cur_out);
		}
	}

	return out;
}

void cStatic3dx::BuildProgramLod(cTempMesh3dx* lod0,cTempMesh3dx*& lod1,cTempMesh3dx*& lod2)
{
/*
	lod1=RealCutMesh(lod0,1);
	lod2=RealCutMesh(lod0,2);
*/
//	lod1=FakeCutMesh(lod0,1);
//	lod2=FakeCutMesh(lod0,2);
	lod1=RealLoadedMesh(lod0,1);
	lod2=RealLoadedMesh(lod0,2);
}

cTempMesh3dx* FakeCutMesh(cTempMesh3dx* lod0,int lod)
{
	cTempMesh3dx* p=new cTempMesh3dx;
	*p=*lod0;
	int size=p->polygons.size();
	size=size>>lod;
	p->polygons.resize(size);
	return p;
}

/// Формат для скининга с 4 костями.
struct sVertexXYZW4INT2
{
	Vect3f	pos;
	float weight[4];/// веса костей
	BYTE index[4];/// индексы матриц
	Vect3f	n;
	float			uv[2];
	float			uv2[2];
	inline Vect2f& GetTexel()			{ return *(Vect2f*)&uv[0]; }
	inline Vect2f& GetTexel2()			{ return *(Vect2f*)&uv2[0]; }
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
};

static D3DVERTEXELEMENT9 sVertexXYZW4INT2_elements[] = {
	{ 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,   D3DDECLUSAGE_POSITION, 0 },
	{ 0, 3*4, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT,   D3DDECLUSAGE_BLENDWEIGHT, 0 },
	{ 0, 7*4, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
	{ 0, 8*4, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,   D3DDECLUSAGE_NORMAL, 0 },
	{ 0, 11*4, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,   D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 13*4, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,   D3DDECLUSAGE_TEXCOORD, 1 },
    D3DDECL_END() 
};

cTempMesh3dx* RealCutMesh(cTempMesh3dx* lod0,int lod)
{
	xassert(sizeof(sVertexXYZW4INT2)==15*4);
	LPD3DXMESH pMesh=NULL;
	RDCALL(D3DXCreateMesh(
		lod0->polygons.size(),
		lod0->vertex_pos.size(),
		D3DXMESH_SYSTEMMEM,
		sVertexXYZW4INT2_elements,
		gb_RenderDevice3D->lpD3DDevice,
		&pMesh
	));

	{
		sPolygon* p=NULL;
		RDCALL(pMesh->LockIndexBuffer(D3DLOCK_DISCARD,(void**)&p));
		memcpy(p,&lod0->polygons[0],lod0->polygons.size()*sizeof(sPolygon));
		pMesh->UnlockIndexBuffer();
	}

	{
		sVertexXYZW4INT2* pvertex;
		RDCALL(pMesh->LockVertexBuffer(D3DLOCK_DISCARD,(void**)&pvertex));
		for(int i=0;i<lod0->vertex_pos.size();i++)
		{
			sVertexXYZW4INT2& p=pvertex[i];
			p.pos=lod0->vertex_pos[i];
			p.n=lod0->vertex_norm.empty()?Vect3f::K:lod0->vertex_norm[i];
			p.GetTexel()=lod0->vertex_uv.empty()?Vect2f::ZERO:lod0->vertex_uv[i];
			p.GetTexel2()=lod0->vertex_uv2.empty()?Vect2f::ZERO:lod0->vertex_uv2[i];
			if(lod0->bones.empty())
			{
				for(int iw=0;iw<4;iw++)
				{
					p.weight[iw]=0;
					p.index[iw]=lod0->inode;
				}
			}else
			{
				for(int iw=0;iw<4;iw++)
				{
					p.weight[iw]=lod0->bones[i].weight[iw];
					p.index[iw]=lod0->bones[i].inode[iw];
				}
			}
		}

		pMesh->UnlockVertexBuffer();
	}

	LPD3DXPMESH pPMesh=NULL;
	DWORD* pAdjacency=new DWORD[lod0->polygons.size()*3];
	//Потом D3DXWeldVertices попробовать
	RDCALL(pMesh->GenerateAdjacency(1e-6f,pAdjacency));

	HRESULT hr=D3DXGeneratePMesh(pMesh,
		pAdjacency,
		NULL,
		NULL,
		max(lod0->polygons.size()/4,8),
		D3DXMESHSIMP_FACE,
		&pPMesh
	);//Все равно не работает - некоторые модели не может создать, а на некоторых внутри падает.

	RELEASE(pMesh);
	delete pAdjacency;

	cTempMesh3dx* lodx=new cTempMesh3dx;
	*lodx=*lod0;

	if(SUCCEEDED(hr))
	{
		pPMesh->SetNumFaces(max(lod0->polygons.size()>>lod,8));

		{
			sPolygon* p=NULL;
			lodx->polygons.resize(pPMesh->GetNumFaces());
			RDCALL(pPMesh->LockIndexBuffer(D3DLOCK_READONLY,(void**)&p));
			memcpy(&lodx->polygons[0],p,lodx->polygons.size()*sizeof(sPolygon));
			pPMesh->UnlockIndexBuffer();
		}


		{
			int num_vertices=pPMesh->GetNumVertices();
			lodx->vertex_pos.resize(num_vertices);
			if(!lod0->vertex_norm.empty())
				lodx->vertex_norm.resize(num_vertices);

			if(!lod0->vertex_uv.empty())
				lodx->vertex_uv.resize(num_vertices);
			if(!lod0->vertex_uv2.empty())
				lodx->vertex_uv2.resize(num_vertices);

			if(!lod0->bones.empty())
				lodx->bones.resize(num_vertices);

			sVertexXYZW4INT2* pvertex;
			RDCALL(pPMesh->LockVertexBuffer(D3DLOCK_DISCARD,(void**)&pvertex));
			for(int i=0;i<num_vertices;i++)
			{
				sVertexXYZW4INT2& p=pvertex[i];
				lodx->vertex_pos[i]=p.pos;
				if(!lodx->vertex_norm.empty())
					lodx->vertex_norm[i]=p.n;
				if(!lodx->vertex_uv.empty())
					lodx->vertex_uv[i]=p.GetTexel();
				if(!lodx->vertex_uv2.empty())
					lodx->vertex_uv2[i]=p.GetTexel2();
				if(!lodx->bones.empty())
				{
					for(int iw=0;iw<4;iw++)
					{
						lodx->bones[i].weight[iw]=p.weight[iw];
						lodx->bones[i].inode[iw]=p.index[iw];
					}
				}
			}
			pPMesh->UnlockVertexBuffer();
		}
	}else
	{
		lodx->polygons.resize(1);
		lodx->polygons[0].set(0,1,2);
		int num_vertices=3;
		lodx->vertex_pos.resize(num_vertices);
		if(!lod0->vertex_norm.empty())
			lodx->vertex_norm.resize(num_vertices);

		if(!lod0->vertex_uv.empty())
			lodx->vertex_uv.resize(num_vertices);
		if(!lod0->vertex_uv2.empty())
			lodx->vertex_uv2.resize(num_vertices);

		if(!lod0->bones.empty())
			lodx->bones.resize(num_vertices);

		dprintf("Failed D3DXGeneratePMesh\n");
	}

	RELEASE(pPMesh);
	return lodx;
}


void cStatic3dx::LoadLod(CLoadDirectory dir,vector<cTempMesh3dx*>& temp_mesh_lod)
{
	IF_FIND_DIR(C3DX_MESHES)
	{
		temp_mesh_lod.clear();
		while(CLoadData* ld=dir.next())
		switch(ld->id)
		{
		case C3DX_MESH:
			{
				cTempMesh3dx* mesh=new cTempMesh3dx;
				mesh->Load(ld);
				if(!mesh->vertex_uv2.empty())
					is_uv2=true;

				if(mesh->name.empty())
				{
					int inode=mesh->inode;
					mesh->name=nodes[inode].name;
				}

				temp_mesh_lod.push_back(mesh);
			}
			break;
		}
	}
}

cTempMesh3dx* cStatic3dx::RealLoadedMesh(cTempMesh3dx* lod0,int lod)
{
	xassert(lod==1 || lod==2);

	vector<cTempMesh3dx*>& temp_mesh=lod==1?temp_mesh_lod1:temp_mesh_lod2;
	vector<cTempMesh3dx*>::iterator it;
	FOR_EACH(temp_mesh,it)
	{
		string& name=(*it)->name;
		if(name==lod0->name)
		{
			if((*it)->imaterial==lod0->imaterial)
			{
				cTempMesh3dx* out=*it;
				temp_mesh.erase(it);
				return out;
			}
		}
	}
	return NULL;
}
