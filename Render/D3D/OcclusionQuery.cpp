#include "StdAfxRD.h"
#include "OcclusionQuery.h"

cOcclusionQuery::cOcclusionQuery()
{
	testedCount_ = 0;
	draw=false;
	pQuery=NULL;
	Init();
	gb_RenderDevice3D->occlusion_query.push_back(this);
}

cOcclusionQuery::~cOcclusionQuery()
{
	RELEASE(pQuery);

	if(gb_RenderDevice3D)
	{
		bool erased=false;
		vector<cOcclusionQuery*>::iterator it;
		FOR_EACH(gb_RenderDevice3D->occlusion_query,it)
		if(*it==this)
		{
			gb_RenderDevice3D->occlusion_query.erase(it);
			erased=true;
			break;
		}

		VISASSERT(erased);
	}
}

bool cOcclusionQuery::Init()
{
	RELEASE(pQuery);
	HRESULT hr;
	if(Option_EnableOcclusionQuery)
		hr=gb_RenderDevice3D->lpD3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION,&pQuery);
	else
		hr=E_FAIL;
	return SUCCEEDED(hr);
}

void cOcclusionQuery::Done()
{
	RELEASE(pQuery);
}

void cOcclusionQuery::Test(const Vect3f* point, int numPoints)
{
	testedCount_ = numPoints;
	if(numPoints==0)
	{
		draw=false;
		return;
	}

	if(true)
	{
		gb_RenderDevice3D->SetNoMaterial(ALPHA_BLEND,MatXf::ID,0,NULL);

		gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
		gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
	}else
	{//тестовый код
		gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,NULL);
	}

	cVertexBuffer<sVertexXYZ>& buf = gb_RenderDevice3D->BufferXYZOcclusion;
	cCamera* pCamera=gb_RenderDevice3D->GetDrawNode();
	int num_visible=0;

	sVertexXYZ* v=(sVertexXYZ*)buf.Lock(numPoints);
	for(int i = 0; i < numPoints; ++i)
	{
		if(pCamera->TestVisible(point[i],0))
		{
			num_visible++;
			(v++)->pos = point[i];
		}
	}
	buf.Unlock(num_visible);

	if(num_visible)
	{
		if(pQuery)
		{
			pQuery->Issue(D3DISSUE_BEGIN);
			buf.DrawPrimitive(PT_POINTLIST, num_visible);
			pQuery->Issue(D3DISSUE_END);
		}
		draw=true;
	}else
		draw=false;

}

void cOcclusionQuery::Test(const Vect3f& sampleCenter)
{
	Test(&sampleCenter,1);
}

int cOcclusionQuery::VisibleCount()
{
	if(!pQuery)
		return 1;
	DWORD point=0;

	if(!draw)
		return 0;

	for(int i=0;i<10;i++)
	{
		if(pQuery->GetData((void *) &point, sizeof(DWORD), D3DGETDATA_FLUSH) != S_FALSE)
			break;
		Sleep(1);//FOR Radeon 7500, 8500
	}

	//В случеае включения multisampling'а на Radeon 9700 возвращается непонятное значение
	// point> 1 && point < gb_RenderDevice3D->GetMultisampleNum()
	if(testedCount_==1)
	{
		point=point?1:0;
	}else
	{
		point/=gb_RenderDevice3D->GetMultisampleNum();
	}

	return point;
}

bool cOcclusionQuery::IsVisible()
{
	return VisibleCount() != 0;
}

void cOcclusionQuery::Begin()
{
	if(pQuery)
		pQuery->Issue(D3DISSUE_BEGIN);
}

void cOcclusionQuery::End()
{
	if(pQuery)
	    pQuery->Issue(D3DISSUE_END);
	draw=true;
}
