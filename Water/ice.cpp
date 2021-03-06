#include "StdAfx.h"
#include "ice.h"
#include "Render\shader\shaders.h"
#include "Serialization\ResourceSelector.h"
#include "Render\Src\TexLibrary.h"
#include "Render\Src\cCamera.h"
#include "Render\D3D\D3DRender.h"

cTemperature::cTemperature()
:BaseGraphObject(0)
{
	data=data_new=0;
	diffusion=0.2f;
	out_temperature=0;
	alpha_ref=30;
	border=0.5f;
	//border_real=border+alpha_ref/255.0f;
	border_real=border+2.75f*alpha_ref/255.0f;;

	texture_=0;
	textureBump_=0;
	textureAlpha_=0;
	textureCleft_=0;
	iceShader_= new ShaderSceneWaterIce;
	iceShader_->Restore();
	change_function = 0;
}

cTemperature::~cTemperature()
{
	delete iceShader_;
	RELEASE(texture_);
	RELEASE(textureAlpha_);
	RELEASE(textureBump_);
	RELEASE(textureCleft_);
	delete[] data;
	delete[] data_new;
}

void cTemperature::SetTexture(const char* snow,const char* bump,const char* cleft)
{
	RELEASE(texture_);
	RELEASE(textureBump_);
	RELEASE(textureCleft_);
	texture_=GetTexLibrary()->GetElement3D(snow);
	textureBump_=GetTexLibrary()->GetElement3D(bump,"Bump");
	textureCleft_=GetTexLibrary()->GetElement3D(cleft);
}

void cTemperature::Init(Vect2i real_size_,int grid_shift_,cWater* pWater_)
{
	pWater=pWater_;
	real_size=real_size_;
	grid_shift=grid_shift_;
	cell_size=1<<grid_shift;
	grid_and=(1<<grid_shift)-1;
	grid_size.x=real_size.x>>grid_shift;
	grid_size.y=real_size.y>>grid_shift;
	xassert((grid_size.x<<grid_shift)==real_size.x);
	xassert((grid_size.y<<grid_shift)==real_size.y);

	data=new float[grid_size.x*grid_size.y];
	data_new=new float[grid_size.x*grid_size.y];

	InitGrid();

	textureAlpha_=GetTexLibrary()->CreateAlphaTexture(grid_size.x,grid_size.y,0,true);
}

void cTemperature::InitGrid()
{
	int x,y;

	for(y=0;y<grid_size.y;y++)
		for(x=0;x<grid_size.x;x++)
		{
			int offset=x+y*grid_size.x;
			data[offset]=out_temperature;
			data_new[offset]=out_temperature;
		}
}

void cTemperature::SetOutIce(bool enable)
{
	if(enable)
		out_temperature = 1;
	else
		out_temperature = 0;
}

float cTemperature::Get(int x,int y) const
{
	int xi=x>>grid_shift;
	int yi=y>>grid_shift;
	if(xi<0 || xi>=grid_size.x-1 ||
		yi<0 || yi>=grid_size.y-1)
		return out_temperature;
	float cx=(x&grid_and)/float(1<<grid_shift);
	float cy=(y&grid_and)/float(1<<grid_shift);

	float* z00=&data[xi+yi*grid_size.x];
	float* z01=z00+1;
	float* z10=z00+grid_size.x;
	float* z11=z10+1;

	return bilinear(*z00,*z01,
					*z10,*z11,
					cx,cy);
}

bool cTemperature::isOnIce(const Vect3f& pos, float radius) const
{
	int xx = pos.xi() >> gridShift();
	int yy = pos.yi() >> gridShift();
	
	if(xx < 0 || xx >= grid_size.x || yy < 0 || yy >= grid_size.y)
		return false;

	if(checkTile(xx, yy))
		return true;

	if(radius > FLT_EPS){
		int xL=round(pos.x-radius)>>gridShift();
		int xR=round(pos.x+radius)>>gridShift();
		int yT=round(pos.y-radius)>>gridShift();
		int yD=round(pos.y+radius)>>gridShift();
		xL=max(xL,0);
		xR=min(xR,grid_size.x-1);

		yT=max(yT,0);
		yD=min(yD,grid_size.y-1);

		for(int y=yT; y<=yD; y++)
			for(int x=xL; x<=xR; x++)
				if(checkTile(x, y))	return true;
	}

	return false;
}

Vect2f cTemperature::GetGradient(int x,int y)
{
	xassert(0);
	return Vect2f::ZERO;
}

void cTemperature::CalcOne(int x,int y)
{
	float div=dget(x-1,y)+dget(x,y-1)+
			  dget(x+1,y)+dget(x,y+1)
			  -4*dget(x,y);

	data_new[x+y*grid_size.x]+=diffusion*div;
}

void cTemperature::CalcOneBorder(int x,int y)
{
	float div=dgetb(x-1,y)+dgetb(x,y-1)+
			  dgetb(x+1,y)+dgetb(x,y+1)
			  -4*dget(x,y);

	data_new[x+y*grid_size.x]+=diffusion*div;
}

void cTemperature::LogicQuant()
{
	int x,y;
	memcpy(data_new,data,grid_size.x*grid_size.y*sizeof(data[0]));
	for(x=0;x<grid_size.x;x++)
	{
		CalcOneBorder(x,0);
		CalcOneBorder(x,grid_size.y-1);
	}

	for(y=0;y<grid_size.y;y++)
	{
		CalcOneBorder(0,y);
		CalcOneBorder(grid_size.x-1,y);
	}

	for(y=1;y<grid_size.y-1;y++)
	for(x=1;x<grid_size.x-1;x++)
	{
		CalcOne(x,y);
	}

	swap(data,data_new);

	for(y=0;y<grid_size.y;y++)
	for(x=0;x<grid_size.x;x++)
	{
		float dold=data[x+y*grid_size.x];
		float dnew=data_new[x+y*grid_size.x];
		bool bold=dold<border;
		bool bnew=dnew<border;
		if(bnew != bold)
			Change(x,y,!bnew);
	}

//	log_var_crc(data, sizeof(float)*grid_size.x*grid_size.y);
//	log_var_crc(data_new, sizeof(float)*grid_size.x*grid_size.y);
}

#define MUL_SHIFT 9
void cTemperature::UpdateColor()
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	int pitch;
	int bpp=textureAlpha_->bitsPerPixel();
	xassert(bpp==8);

	BYTE* out_data=textureAlpha_->LockTexture(pitch);
	for(int y=0;y<grid_size.y;y++)
	{
		BYTE* p=out_data+y*pitch;
		for(int x=0;x<grid_size.x;x++,p++)
		{
			/*float* z00=&data[x+y*grid_size.x];
			float p00=mul*(*z00-border);
			p00=clamp(p00,0,255);
			*p=p00;
			/**/
			int	p00=clamp(round((data[x+y*grid_size.x]-border)*(1<<MUL_SHIFT)),0,255);
			//int p00 = 255;
			*p=p00;
			/**/
		}
	}
	textureAlpha_->UnlockTexture();

}


void cTemperature::DebugShow()
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetNoMaterial(ALPHA_BLEND,MatXf::ID,0);
	cQuadBuffer<sVertexXYZDT1>* quad=rd->GetQuadBufferXYZDT1();
	quad->BeginDraw();
	float mul=255;
	for(int yi=0;yi<grid_size.y-1;yi++)
	for(int xi=0;xi<grid_size.x-1;xi++)
	{
		int offset=xi+yi*grid_size.x;
		int xx=xi<<grid_shift;
		int yy=yi<<grid_shift;
		float* z00=&data[xi+yi*grid_size.x];
		float* z01=z00+1;
		float* z10=z00+grid_size.x;
		float* z11=z10+1;
		float p00=mul**z00,p01=mul**z01,p10=mul**z10,p11=mul**z11;
		
		sVertexXYZDT1* vt=quad->Get();
		vt[0].pos.set(xx,yy,0);
		vt[0].diffuse=Color4c(p00,p00,p00,255);
		vt[1].pos.set(xx,yy+cell_size,0);
		vt[1].diffuse=Color4c(p10,p10,p10,255);
		vt[2].pos.set(xx+cell_size,yy,0);
		vt[2].diffuse=Color4c(p01,p01,p01,255);
		vt[3].pos.set(xx+cell_size,yy+cell_size,0);
		vt[3].diffuse=Color4c(p11,p11,p11,255);
		vt[0].GetTexel().set(0,0);
		vt[1].GetTexel().set(0,0);
		vt[2].GetTexel().set(0,0);
		vt[3].GetTexel().set(0,0);
	}

	quad->EndDraw();
}

void cTemperature::Draw(Camera* cameraX)
{
	UpdateColor();

	iceShader_->beginDraw(texture_, textureBump_, textureAlpha_, textureCleft_, alpha_ref, out_temperature == 1 ? 0xFFFFFFFF : 0);

	pWater->DrawPolygons(gb_RenderDevice3D->camera());

	iceShader_->endDraw();
}

void cTemperature::SetT(int x,int y,float t,int size)
{
	x=x>>grid_shift;
	y=y>>grid_shift;
	size=(size+grid_shift-1)>>grid_shift;
	int xmin=max(x-size,0);
	int ymin=max(y-size,0);
	int xmax=min(x+size,grid_size.x-1);
	int ymax=min(y+size,grid_size.y-1);
	for(int yy=ymin;yy<=ymax;yy++)
	for(int xx=xmin;xx<=xmax;xx++)
	{
		xassert(xx>=0 && xx<grid_size.x);
		xassert(yy>=0 && yy<grid_size.y);

		float dold=data[xx+yy*grid_size.x];
		float dnew=t;
		bool bold=dold<border;
		bool bnew=dnew<border;

		data[xx+yy*grid_size.x]=t;

		if(bnew != bold) 
			Change(xx,yy,!bnew);
	}
}

void cTemperature::Change(int x,int y,bool ice)
{
	if(change_function)
		change_function(x,y);
}

void cTemperature::PreDraw(Camera* camera)
{
	camera->Attach(SCENENODE_OBJECTSPECIAL,this);
}

void cTemperature::Animate(float dt)
{
}

void cTemperature::serialize(Archive& ar)
{
	Vect2i grid_size_in = grid_size;
	ar.serialize(grid_size_in, "grid_size", 0);
	if(grid_size_in == grid_size)
		ar.serialize(MemoryBlock(data, grid_size.x*grid_size.y*sizeof(data[0])), "zbuf", 0);
}

////////////////////cClusterIndexBuffer////////////////////////
cClusterIndexBuffer::cClusterIndexBuffer()
{
	num_tiles=0;
	data=0;
	end_cluster=0;
}

cClusterIndexBuffer::~cClusterIndexBuffer()
{
}

bool cClusterIndexBuffer::Init(int num_tiles_)
{
	num_tiles=num_tiles_;
	end_cluster=num_tiles;

	realloc_table.resize(num_tiles);
	for(int i=0;i<num_tiles;i++)
	{
		Realloc& r=realloc_table[i];
		r.logic_to_physic=i;
		r.physic_to_logic=i;
		r.cluster=0;
	}

	gb_RenderDevice->CreateIndexBuffer(ib,num_tiles*2);
	return true;
}

void cClusterIndexBuffer::LockSetTile()
{
	sPolygon* ptr=gb_RenderDevice->LockIndexBuffer(ib);	
	data=(sTile*)ptr;

	vector<DeferredSetTile>::iterator it;
	FOR_EACH(deffered_set,it)
		SetTile(it->idx_tile,it->idx_cluster);
	deffered_set.clear();
}

void cClusterIndexBuffer::UnlockSetTile()
{
	gb_RenderDevice->UnlockIndexBuffer(ib);
	data=0;
}

void cClusterIndexBuffer::SetTile(int idx_tile,bool idx_cluster)
{
	if(!data)
	{
		DeferredSetTile d;
		d.idx_cluster=idx_cluster;
		d.idx_tile=idx_tile;
		deffered_set.push_back(d);
		return;
	}
	xassert(idx_tile>=0 && idx_tile<num_tiles);
	Realloc& cur=realloc_table[idx_tile];
	if(cur.cluster==idx_cluster)
		return;

	cur.cluster=idx_cluster;
	int border=idx_cluster?end_cluster-1:end_cluster;

	if(cur.logic_to_physic!=border)
	{
		CheckLink(idx_tile);
		CheckLink(border);
		Realloc& to_swap=realloc_table[border];
		int idx_swap=to_swap.physic_to_logic;
		swap(data[cur.logic_to_physic],data[border]);

		int end_physic_to_logic=realloc_table[border].physic_to_logic;
		swap(realloc_table[cur.logic_to_physic].physic_to_logic,realloc_table[border].physic_to_logic);
		realloc_table[end_physic_to_logic].logic_to_physic=cur.logic_to_physic;
		cur.logic_to_physic=border;

		CheckLink(idx_tile);
		CheckLink(border);
	}

	idx_cluster?end_cluster--:end_cluster++;
}

void cClusterIndexBuffer::CheckLink(int logic_idx)
{
	int physic_idx=realloc_table[logic_idx].logic_to_physic;
	xassert(realloc_table[physic_idx].physic_to_logic==logic_idx);
}


