#include "StdAfxRD.h"
#include "Leaves.h"
#include "Scene.h"
#include "cCamera.h"
#include "D3DRender.h"

Leaves::Leaves(): cUnkObj(KIND_LEAVES)
{
	texture_ = 0;
	lod = 0;
	distanceLod12_ = 1000000;
	distanceLod23_ = 225000;
	lodChangeDistance_ = 25000;
}
Leaves::~Leaves()
{
	for(int i=0; i<leaves_.size(); i++)
	{
		delete leaves_[i];
	}
	leaves_.clear();
	RELEASE(texture_);
}
void Leaves::Animate(float dt)
{
}
void Leaves::PreDraw(Camera* camera)
{
	sortedLeaves_.clear();
	if(!camera->TestVisible(GlobalMatrix,bound_.min,bound_.max))
		return;
	float dist=camera->GetPos().distance2(GlobalMatrix.trans());
	if (dist<distanceLod12_)
	{
		lod = 0;
	}else
	if (dist<distanceLod23_)
	{
		lod = 1;
	}else
	{
		lod = 2;
	}
	for(int i=0; i<lod_leaves_[lod].size(); i++)
	{
		Leaf* leaf = lod_leaves_[lod][i];
		SortedLeaf sleaf;
		sleaf.leaf = leaf;
		Vect3f lightDir;
		camera->GetLighting(lightDir);
		//Vect3f pos;
		//pos = leaf->pos.trans()-lightDir*leaf->size;
		//float a=1;
		//for(int j=0; j<leaves_.size(); j++)
		//{
		//	Leaf* leaf1 = leaves_[j];
		//	if(leaf1==leaf)
		//		continue;
		//	a -= Intersect(leaf1->pos.trans(),leaf1->size,pos,-lightDir);
		//}
		//if(a<0) a=0;
		//leaf->color *= 0.4f+a*0.6f;
		sleaf.distance  = lightDir.dot(leaf->pos.trans());
		//sleaf.distance = camera->GetPos().distance2(leaf->pos.trans());
		//sleaf.distance = GlobalMatrix.trans().distance2(leaf->pos.trans());
		sortedLeaves_.push_back(sleaf);
	}
	camera->Attach(SCENENODE_OBJECT,this);
}
float Leaves::Intersect(Vect3f& pos1,float radius, Vect3f& pos, Vect3f& dir)
{
	float r1 = radius;
	Vect3f C = pos1;
	Vect3f B = C-pos;
	if(B.dot(dir) < 0.f)
		return 0.f;
	float r2 = B.postcross(dir).norm();
	if(r2>r1)
		return 0.f;
	return 1.f-r2/r1;
	//return -1.f;
}

void Leaves::CalcLeafColor()
{
	//u from -N to N
	//v from 0 to M

	//x(u,v) = R * cos( u * pi / 2N ) * cos( v * 2pi / N )
	//y(u,v) = R * cos( u * pi / 2N ) * sin( v * 2pi / N )
	//z(u,v) = R * sin( u * pi / 2N )

	Color4f color(1,1,1,1);
	for(int l=0; l<StaticVisibilitySet::num_lod; l++)
	for(int i=0; i<leaves_.size(); i++)
	{
		Leaf* leaf = leaves_[i];
		//Vect3f center = bound_.center()+GetPosition().trans();
		//Vect3f pos = leaf->pos.trans();
		//Vect3f n = pos-center;
		//n.normalize();
		//leaf->normal = n;
		//leaf->normal.normalize();
		float sum=0;
		int rayCount = 0;
		/**/
		for(int u=0; u<90; u+=4)
		{
			for(int v=0; v<360; v+=4)
			{
				Vect3f dir;
				dir.x = cosf(G2R(u)) * cosf(G2R(v)*4);
				dir.y = cosf(G2R(u)) * sinf(G2R(v)*4);
				dir.z = sinf(G2R(u));
				Vect3f pos;
				pos = leaf->pos.trans()+dir*leaf->size;

				
				float a=1;
				for(int j=0; j<leaves_.size(); j++)
				{
					Leaf* leaf1 = leaves_[j];
					if(leaf1==leaf)
						continue;
					a -= Intersect(leaf1->pos.trans(),leaf1->size,pos,dir);
				}
				if(a>0)
					sum += a;
				rayCount++;
			}
		}
		/**/
		for(int u=-90; u<0; u+=4)
		{
			for(int v=0; v<360; v+=4)
			{
				Vect3f dir;
				dir.x = cosf(G2R(u)) * cosf(G2R(v)*4);
				dir.y = cosf(G2R(u)) * sinf(G2R(v)*4);
				dir.z = sinf(G2R(u));
				Vect3f pos;
				pos = leaf->pos.trans()+dir*leaf->size;

				float a=1;
				for(int j=0; j<leaves_.size(); j++)
				{
					Leaf* leaf1 = leaves_[j];
					if(leaf1==leaf)
						continue;
					a -= Intersect(leaf1->pos.trans(),leaf1->size,pos,dir)*0.5f;
				}
				if(a>0)
					sum += a;
				rayCount++;
			}
		}
		/**/
		sum /= rayCount;
		leaf->color = color*sum;
	}
}

void Leaves::Draw(Camera* camera)
{
	stable_sort(sortedLeaves_.begin(),sortedLeaves_.end(),LeafSortByRadius());
	DWORD oldCull = gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
	//int firstLod=0;
	//float distanceLod = distanceLod12_;
	//float dist=camera->GetPos().distance2(GetPosition().trans());

	//if(dist>distanceLod12_)
	//{
	//	firstLod=1;
	//	distanceLod = distanceLod23_;
	//}
	//float alpha=1.f;
	//for(int l=0; l<2; l++)
	//{
	//	if(l==0)
	//	{
	//		alpha = 1.f-(dist-distanceLod+lodChangeDistance_)/lodChangeDistance_;
	//		alpha = clamp(alpha,0.f,1.f);
	//		if(alpha<FLT_EPS)
	//			continue;
	//	}else
	//	{
	//		alpha = (dist-distanceLod+lodChangeDistance_)/lodChangeDistance_;
	//		alpha = clamp(alpha,0.f,1.f);
	//		if(alpha<FLT_EPS)
	//			continue;
	//	}
	Vect3f lightDir;
	Color4f sunDiffuse = scene()->GetSunDiffuse();
	sunDiffuse.r = clamp(sunDiffuse.r,0.f,1.f);
	sunDiffuse.g = clamp(sunDiffuse.g,0.f,1.f);
	sunDiffuse.b = clamp(sunDiffuse.b,0.f,1.f);
	sunDiffuse.a = clamp(sunDiffuse.a,0.f,1.f);
	//camera->GetLighting(lightDir);
	//Vect3f n(0,0,1);
	cQuadBuffer<sVertexXYZDT1>* pBuf=gb_RenderDevice->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	gb_RenderDevice3D->SetWorldMaterial(ALPHA_BLEND,MatXf::ID,0,texture_);//???
	gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_TEST);
	for(int i=0; i<sortedLeaves_.size(); i++)
	{
		Leaf* leaf = sortedLeaves_[i].leaf;
		//float dist=camera->xformZ(leaf->pos.trans());
		//Vect3f cameraDir = camera->GetMatrix().trans()-leaf->pos.trans();
		//Vect3f n = (leaf->normal - cameraDir)*0.5f;
		//float a = leaf->normal.dot(-lightDir);
		//float a = n.dot(-lightDir);
		//Vect3f pos1 = leaf->pos.trans()+leaf->normal*10;
		//gb_RenderDevice3D->DrawLine(leaf->pos.trans(),pos1,Color4c(255,255,0,255));
		float a = float(i)/sortedLeaves_.size();
		a = clamp(a,0.5f,1.f);
		Color4f color = leaf->color*sunDiffuse*a;
		//color *= ;
		//color.a =alpha;
		color.a =1.f;
		Color4c Diffuse(color);

		sVertexXYZDT1 *v=pBuf->Get();
		Vect3f sx=leaf->size*camera->GetWorldI(),sy=leaf->size*camera->GetWorldJ();
		v[0].pos=leaf->pos.trans()+sx+sy; v[0].u1()=0, v[0].v1()=0; 
		v[1].pos=leaf->pos.trans()+sx-sy; v[1].u1()=0, v[1].v1()=1;
		v[2].pos=leaf->pos.trans()-sx+sy; v[2].u1()=1, v[2].v1()=0;
		v[3].pos=leaf->pos.trans()-sx-sy; v[3].u1()=1, v[3].v1()=1;
		v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=Diffuse;
	}
	pBuf->EndDraw();
	//}
	gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,oldCull);
}
Leaf* Leaves::AddLeaf()
{
	Leaf* leaf = new Leaf();
	leaves_.push_back(leaf);
	return leaf;
}
int Leaves::GetLeavesCount()
{
	return leaves_.size();
}
Leaf* Leaves::GetLeaf( int i )
{
	xassert(i<leaves_.size());
	return leaves_[i];
}
void Leaves::SetTexture( cTexture* texture )
{
	texture_ = texture;
	if(!texture)
		return;
	texture_->AddRef();
}
void Leaves::CalcBound()
{
	bound_.min.set(0,0,0);
	bound_.max.set(0,0,0);
	for(int i=0; i<leaves_.size(); i++)
	{
		bound_.addPoint(leaves_[i]->pos.trans()-GlobalMatrix.trans());
	}
}
void Leaves::SetLod(int ilod)
{
	lod = ilod;
}
void Leaves::CalcLods(StaticVisibilityGroup::VisibleNodes& visible_nodes, int lod)
{
	xassert(lod>=0&&lod<StaticVisibilitySet::num_lod);
	for(int i=0; i<visible_nodes.size(); i++)
	{
		if(!visible_nodes[i])
			continue;
		for(int j=0; j<leaves_.size(); j++)
		{
			if(leaves_[j]->inode == i)
			{
				lod_leaves_[lod].push_back(leaves_[j]);
				break;
			}
		}
	}
}
void Leaves::SetDistanceLod(float distanceLod12,float distanceLod23)
{
	distanceLod12_ = distanceLod12;
	distanceLod23_ = distanceLod23;
	lodChangeDistance_ = distanceLod12_/2;
}
