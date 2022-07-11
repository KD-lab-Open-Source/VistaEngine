#include "stdafx.h"
#include "..\Water\Water.h"
#include "..\Water\Waves.h"
#include "ice.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "RenderObjects.h"
#include "ExternalShow.h"

static float mz = 1;
cWaves::cWaves() : cBaseGraphObject(0)
{
	gran = NULL;
	Texture = NULL;
	active = false;
	pena = NULL;
	pScene = NULL;
}

cWaves::~cWaves()
{
	RELEASE(Texture);
}
void cWaves::SetTexture(const char* name)
{ 
	RELEASE(Texture);
	//Texture = GetTexLibrary()->GetElement(name);	  
	Texture = GetTexLibrary()->GetElement3D(name);	  
}
void cWaves::Init(cWater* pWater, cScene* scene)
{
	pScene = scene;
	fronts.clear();
	F_ebb = 0;
	xassert(pWater);
	T = 3;
	Fmax = 0.2f;
//	points.clear();
//	points.push_back(list<WAVE>());
//	list<WAVE>& con = points.back();
	this->pWater = pWater;
	static float t = 20;
	wide = t;
	grid.x = pWater->GetGridSizeX();
	grid.y = pWater->GetGridSizeY();
	float z = pWater->GetEnvironmentWater();
	delete[] gran;
	gran = new GRID_DATA[grid.x*grid.y];
	int y=0;
	bord_count = 0;
//	for(int y=0;y<grid.y;y++)
	{
		BORDER t;
		t.z0.set(0,y);
		t.nz1 = true;
		gran[y*grid.x] = W_earth;
		bord.push_back(t);
/*
		t.nz1 = true;
		t.z0.set((grid.x-1),y);
		gran[(grid.x-1) + y*grid.x] = W_earth;
		t.dir.set(1,0);
		bord.push_back(t);*/
	}
/*	for(int x=0;x<grid.x;x++)
	{
		BORDER t;
		t.z0.set(x,0);
		t.nz1 = true;
		gran[x] = W_earth;
		t.dir.set(0,-1);
		bord.push_back(t);

		t.z0.set(x,(grid.y-1));
		t.nz1 = true;
		gran[x+(grid.y-1)*grid.x] = W_earth;
		t.dir.set(0,1);
		bord.push_back(t);
	}
*/
	ix[0].set(-1,0);
	ix[1].set(0,1);
	ix[2].set(1,0);
	ix[3].set(0,-1);

	ix[4].set(-1,1);
	ix[5].set(1,1);
	ix[6].set(1,-1);
	ix[7].set(-1,-1);
	SetTexture("scripts\\resource\\Textures\\wave.tga");
}

void cWaves::PreDraw(cCamera *pCamera)
{
	if (active)
		pCamera->Attach(SCENENODE_OBJECTSORT,this);
}	

cWaves::BORDER* cWaves::NextPoint(cWaves::BORDER* last, bool& again)
{
	BORDER* ret = NULL;
	again = false;
	for(int i=0;i<8;i++)
	{
		int x = last->z0.x+ix[i].x;
		int y = last->z0.y+ix[i].y;
		if ((UINT)x>=grid.x || (UINT)y>=grid.y) continue;
		BORDER* t = gran[x+y*grid.x].bord;
		if (t && !t->passed)
			if (ret)
			{
				crotch.push_back(ret);
				return ret;
			}
			else
			{
				t->passed = true;
				ret  = t;
			}
	}
	if (!ret && !crotch.empty())
	{
		again = true;
		ret = crotch.back();
		crotch.pop_back();
	}
	return ret;
}
void cWaves::CalcContour(vector<ContourPt>& contour, vector<WaveLine>& front)
{
	int sz = contour.size();
	if (sz<4)	
		return;
	vector<ContourPt>::iterator it = contour.begin() + 1;
	vector<ContourPt>::iterator end = contour.end() - 1;
	Vect2i v1 = it->pos - (it-1)->pos; 
	v1.set(-v1.y, v1.x);
	bool dir = !((UINT)(v1.x+it->pos.x)<grid.x && (UINT)(v1.y+it->pos.y)<grid.y && pWater->Get(v1.x+it->pos.x, v1.y+it->pos.y).z<mz );
	v1 = (it+1)->pos - it->pos; 
	v1.set(-v1.y, v1.x);
	bool dir2 = !((UINT)(v1.x+it->pos.x)<grid.x && (UINT)(v1.y+it->pos.y)<grid.y && pWater->Get(v1.x+it->pos.x, v1.y+it->pos.y).z<mz );
	if (dir!=dir2)
	{
		v1 = (it+2)->pos - (it+1)->pos; 
		v1.set(-v1.y, v1.x);
		dir = !((UINT)(v1.x+it->pos.x)<grid.x && (UINT)(v1.y+it->pos.y)<grid.y && pWater->Get(v1.x+it->pos.x, v1.y+it->pos.y).z<mz );
	}
	for(;it!=end; it++)
	{
		vector<ContourPt>::iterator prev = it-1;
		vector<ContourPt>::iterator next = it+1;
		Vect2i v1 = it->pos - prev->pos; 
		if (dir) v1.set(-v1.y, v1.x);
		else v1.set(v1.y, -v1.x);

		Vect2i v2 = next->pos - it->pos; 
		if (dir) v2.set(-v2.y, v2.x);
		else v2.set(v2.y, -v2.x);

		it->dir = v1+v2;
		it->dir.normalize(100);
	}
	{
		it = contour.begin();
		Vect2i v1 = (it+1)->pos - it->pos; 
		if (dir) v1.set(-v1.y, v1.x);
		else v1.set(v1.y, -v1.x);
		it->dir = v1;
		it->dir.normalize(100);
	}
	{
		it = end;
		Vect2i v1 = it->pos - (it-1)->pos; 
		if (dir) v1.set(-v1.y, v1.x);
		else v1.set(v1.y, -v1.x);

		it->dir = v1;
		it->dir.normalize(100);
	}
	vector<ContourPt>::iterator p1 = contour.begin();
	vector<ContourPt>::iterator p2 = p1+1;
	for(;p2!=contour.end(); p2++, p1++)
	{
		WaveLine w;
		int shift = pWater->GetCoordShift();
		float z = pWater->GetZ(p1->pos.x, p1->pos.y) + 5;
		w.rect[0].set(p1->pos.x<<shift, p1->pos.y<<shift, z);
		w.rect[1].set(p2->pos.x<<shift, p2->pos.y<<shift, z);
		Vect2f t((w.rect[0].x + p1->dir.x), (w.rect[0].y + p1->dir.y));
		float dz = z;
		z = pWater->GetZ(round(t.x)>>shift, round(t.y)>>shift);
		w.rect[2].set(t.x, t.y, z);
		t.set((w.rect[1].x + p2->dir.x), (w.rect[1].y + p2->dir.y));
		w.rect[3].set(t.x, t.y, z);
		front.push_back(w);
	}
}
void cWaves::CreateFrontWave(vector<WaveLine>& front)
{
	front.clear();
	list<BORDER>::iterator itr;
	FOR_EACH(bord, itr)
		itr->passed = false;
	BORDER* it = &bord.front();
	BORDER* next;
	vector<ContourPt> contour;

	FOR_EACH(bord, itr)
	{
		it = &(*itr);
		BORDER* beg = it;
		BORDER* prev = beg;
		bool contin = false;
		crotch.clear();
		if (itr->passed)
			continue;
		else 
		for(int i=0;i<8;i++)
		{
			int x = it->z0.x+ix[i].x;
			int y = it->z0.y+ix[i].y;
			if ((UINT)x>=grid.x || (UINT)y>=grid.y) continue;
			BORDER* t = gran[x+y*grid.x].bord;
			if (t && itr->passed)
				contin = true;
		}
		if (contin) continue;
		contour.clear();
		for(;;)
		{
			static int cs = 3;
			bool again;
			BORDER* t = it;
			for(int i=0;i<cs;i++)
			{
				next = NextPoint(t,again);
				if (again)
				{
					t = it = next;
					i=0;
					CalcContour(contour, front);
					contour.clear();
				}
				if (next == NULL)
					break;
				t = next;
			}
			if (next == NULL)
				break;
			ContourPt cp;
			cp.pos = it->z0;
			contour.push_back(cp);
			it = next;
		}
		CalcContour(contour, front);
	}
}
void cWaves::CalcBorder()
{
//	list<WAVE>& con = points.back();
	int count=0;
	static int efs = 300;
	int max_count = dt*efs;
	VBRT it;
	FOR_EACH(bord, it)
	{
		BORDER *itt = &(*it);
		int mode = W_unk;
		if (pWater->Get(it->z0.x, it->z0.y).z>=mz)
			mode = W_earth;
		else if (it->nz1)
		{
			for(int i=0;i<4;i++)
			{
				int x = it->z0.x+ix[i].x;
				int y = it->z0.y+ix[i].y;
				if ((UINT)x<grid.x&&(UINT)y<grid.y&&pWater->Get(x, y).z>=mz) 
				{
					it->z1.set(x,y);
					bord_count++;
					goto cont;
				}
			}
			it->z1 =it->z0;
			it->nz1 = false;
			continue;
cont:
			static int nn = 10;
			if (pena && count<max_count&&pWater->Get(it->z1.x, it->z1.y).z>nn)
			{
				cEffect* eff = pScene->CreateEffectDetached(*pena, NULL, true);		
				eff->SetPosition(MatXf(Mat3f::ID,Vect3f(it->z1.x<<pWater->GetCoordShift(),it->z1.y<<pWater->GetCoordShift(),0)));
				attachSmart(eff);
				count++;
			}
			it->nz1 = false;
			continue;
		}
		else if (pWater->Get(it->z1.x, it->z1.y).z<mz)
			mode = W_water;
		else continue;
		int i1 = it->z0.x+ it->z0.y*grid.x;
		gran[i1] = mode == W_earth ? W_water : W_earth;
		for(int i=0;i<4;i++)
		{
			int x = it->z0.x+ix[i].x;
			int y = it->z0.y+ix[i].y;
			if ((UINT)x>=grid.x) continue;
			if ((UINT)y>=grid.y) continue;
			int i2 = x+ y*grid.x;
			if(gran[i2]==mode)
			{
				BORDER n;
				n.z0.set(x,y);
				n.nz1 = true;
				bord.push_back(n);
				gran[i2] = 0;
				gran[i2].bord = &bord.back();
			}
		}
		gran[i1].bord = NULL;
		it = bord.erase(it);
		it--;
	}
}
void cWaves::CreateWave()
{
	fronts.push_back(FRONT());
	FRONT& fr = fronts.back();
	fr.F = Fmax;
	fr.k2 = .2f;
	fr.k1 = 0;
	CreateFrontWave(fr.waves);
	
}
void cWaves::Draw(cCamera *pCamera)
{	
	start_timer_auto();
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetWorldMaterial(ALPHA_BLEND,MatXf::ID, 0, Texture);
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
//	vector<list<WAVE> >::iterator vl;
//	list<WAVE>& con = points.back();
//	vector<WaveLine>::iterator it;
//	static float vel = 0.2f;
	static sColor4c color(255,255,255);
	CalcBorder();
	static float cT = 0;
	if (cT>T)
	{
		CreateWave();
		cT-=T;
	}
	cT+=dt;
	vector<FRONT>::iterator front;
	float ebb = 0;
	FOR_EACH(fronts, front)
	{
		front->k2 += dt*front->F;
		static float kd = 0.001f;
		if (front->F<kd && front->k2<1)
		{
			front = fronts.erase(front);
			front--;
			continue;
		}
		if (front->max_k2<front->k2)
			front->max_k2 = front->k2;
		static float k_ebb = 0.3f;
		if (front->k2>1)
		{
			color.a = (front->k2-1)/(front->max_k2-1)*255;
			front->k1 += dt*F_ebb*k_ebb;
		}
		else 
		{
			color.a = front->k1*255;
			front->k1 += dt*front->F;
		}
		vector<WaveLine>::iterator wave;
		FOR_EACH(front->waves, wave)
		{
			Vect3f p3 = wave->rect[0]*front->k1 + wave->rect[2]*(1-front->k1);
			Vect3f p1 = wave->rect[0]*front->k2 + wave->rect[2]*(1-front->k2);
			Vect3f p4 = wave->rect[1]*front->k1 + wave->rect[3]*(1-front->k1);
			Vect3f p2 = wave->rect[1]*front->k2 + wave->rect[3]*(1-front->k2);
			static float dz = 3;
			p1.z = pWater->GetZ(round(p1.x), round(p1.y))+dz;
			p2.z = pWater->GetZ(round(p2.x), round(p2.y))+dz;
			p3.z = pWater->GetZ(round(p3.x), round(p3.y))+dz;
			p4.z = pWater->GetZ(round(p4.x), round(p4.y))+dz;
			static float bk = 10;
			if (p1.z-p3.z>bk || p2.z-p4.z>bk)
			{
				p1.z = p3.z + bk;
				p2.z = p4.z + bk;
			}
			sVertexXYZDT1 *v=pBuf->Get();
			v[0].pos = p1;	v[0].diffuse=color; 
			v[1].pos = p3;	v[1].diffuse=color; 
			v[2].pos = p2;	v[2].diffuse=color; 
			v[3].pos = p4;	v[3].diffuse=color; 
			v[0].GetTexel().set(0, 0);	
			v[1].GetTexel().set(0, 1);
			v[2].GetTexel().set(1,0);	
			v[3].GetTexel().set(1,1);
		}
		if (front->k2>1)
		{
			front->F+=dt*F_ebb;
			F_ebb -= dt*front->F;
			if (front->k2<front->max_k2&&front->F>0)
				front->F = -F_ebb*k_ebb;
		}
	}
/*
	static bool regions = false;
	vector<WaveLine>::iterator wl;
	if (regions)
	FOR_EACH(front[0], wl)
	{
		float ez = pWater->GetEnvironmentWater()+1;
		Vect3f p1(wl->rect[0].x, wl->rect[0].y,ez);
		Vect3f p2(wl->rect[2].x, wl->rect[2].y,ez);
		rd->DrawLine(p1, p2, sColor4c(255,0,0));
		p1.set(wl->rect[3].x, wl->rect[3].y,ez);
		rd->DrawLine(p2, p1, sColor4c(255,0,0));
		p2.set(wl->rect[1].x, wl->rect[1].y,ez);
		rd->DrawLine(p1, p2, sColor4c(255,0,0));
		p1.set(wl->rect[0].x, wl->rect[0].y,ez);
		rd->DrawLine(p2, p1, sColor4c(255,0,0));
	}
	static bool border = false;
	if (border)
	{
		VBRT it;
		float ez = pWater->GetEnvironmentWater()+1;
		sColor4c beg(0,0,0);
		sColor4c end(255,255,255);
		float sz = bord.size();
		float i = 0;
		FOR_EACH(bord, it)
		{
			Vect3f pos(it->z0.x<<pWater->GetCoordShift(), it->z0.y<<pWater->GetCoordShift(), ez);
			Vect3f sx(10, 0, 0);
			Vect3f sy(0, 16, 0);
			sVertexXYZDT1 *v=pBuf->Get();
			sColor4c color(0,0,0);
			v[0].pos=pos-sx;	v[0].diffuse=color; 
			v[1].pos=pos-sx+sy;	v[1].diffuse=color; 
			v[2].pos=pos+sx;	v[2].diffuse=color; 
			v[3].pos=pos+sx+sy;	v[3].diffuse=color; 

			v[0].GetTexel().set(0, 0);	
			v[1].GetTexel().set(0, 1);
			v[2].GetTexel().set(1,0);	
			v[3].GetTexel().set(1,1);
		}
	}
*/
	pBuf->EndDraw();
}

void cWaves::Animate(float dt)
{
	this->dt = dt*1e-3f;
}

void cWaves::serialize (Archive& ar)
{
	ar.serialize(T, "Period", "Период");
//	ar.serialize(RangedWrapperf(Fmax, 0, 1, 0.01f), "Wave_velocity", "Скорость волн");
	string texture;
	if (Texture)
		texture = Texture->GetName();
	ar.serialize(ResourceSelector (texture, ResourceSelector::TEXTURE_OPTIONS),
								"wave_tga_texture", "Текстура");
	ar.serialize(active, "waves_on", "Включить");
	if (ar.isInput())
		SetTexture(texture.c_str());
}
/////////////////////////////////////////////////////////////////////////////////
cFixedWaves::cFixedWaves()
{
	selectedPoint_ = -1;
	parent_ = NULL;
	texture_ = NULL;
	generationTime_ = 5;
	waveSize_ = Vect2f(40.0f,20.0f);
	distance_ = 100;
	invert_ = 1;
	speed_ = 10.f;
	time_ = 0;
	scaleMin_ = 1.0f;
	scaleMax_ = 1.0f;
	sizeMin_ = 40.0f;
	sizeMax_ = 40.0f;
	textureAnimationTime_ =1.f;

	speedKeys_.InsertKey(0).f = 1.f;

	sizeXKeys_.InsertKey(0).f = 1.f;
	sizeXKeys_.InsertKey(0.5f).f = 2.f;
	sizeXKeys_.InsertKey(1).f = 3.f;

	sizeYKeys_.InsertKey(0).f = 1.f;
	sizeYKeys_.InsertKey(0.5f).f = 2.f;
	sizeYKeys_.InsertKey(1).f = 1.f;

	sizeYKeysCoast_.InsertKey(0).f = 1.f;
	sizeYKeysCoast_.InsertKey(0.5f).f = 3.f;
	sizeYKeysCoast_.InsertKey(1).f = 2.5f;

	alphaKeys_.InsertKey(0).f = 0;
	alphaKeys_.InsertKey(1).f = 1.f;

}

cFixedWaves::~cFixedWaves()
{
	RELEASE(texture_);
}

void cFixedWaves::ShowInfo(sColor4c& color)
{
	sColor4c pntColor = sColor4c(255,255,255);
	for (int i=0; i<points_.size(); i++)
	{
		Vect3f pos = points_[i];
		if (i == selectedPoint_)
			pntColor = sColor4c(255,0,0);
		else
			pntColor = sColor4c(255,255,255);
		gb_RenderDevice->DrawLine(Vect3f(pos.x-10,pos.y,pos.z),Vect3f(pos.x+10,pos.y,pos.z),pntColor);
		gb_RenderDevice->DrawLine(Vect3f(pos.x,pos.y-10,pos.z),Vect3f(pos.x,pos.y+10,pos.z),pntColor);
	}
	for (int i=1; i<points_.size(); i++)
	{
		gb_RenderDevice->DrawLine(points_[i-1],points_[i],color);
	}

	for (int i=1; i<points_.size(); i++)
	{
		
		Vect3f p = points_[i]-points_[i-1];
		float dst = p.norm()/2;
		p.Normalize();
		Vect3f s = p%Vect3f::K;
		s.Normalize();
		gb_RenderDevice->DrawLine(points_[i-1]+p*dst,points_[i-1]+p*dst+s*10*invert_,color);
	}
	for (int i=0; i<segments_.size(); i++)
	{
		gb_RenderDevice->DrawLine(segments_[i].begin-segments_[i].T1*distance_, segments_[i].end-segments_[i].T2*distance_,sColor4c(255,255,0));
		//gb_RenderDevice->DrawLine(segments_[i].begin, segments_[i].begin-segments_[i].T1*distance_,color);
		//gb_RenderDevice->DrawLine(segments_[i].end, segments_[i].end-segments_[i].T2*distance_,color);
	}
}

void cFixedWaves::AddPoint(const Vect3f &point)
{
	points_.push_back(point);
	CreateSegments();
}

void cFixedWaves::CreateSegments()
{
	if (points_.size() <= 1)
		return;
	segments_.clear();
	for (int i=1; i<points_.size(); i++)
	{
		OneSegment segment;
		Vect3f p = points_[i]-points_[i-1];
		segment.lenght = p.norm();
		p.Normalize();
		Vect3f s = p%Vect3f::K;
		s.Normalize();
		segment.dest = s*invert_;
		segment.begin = points_[i-1];
		segment.end = points_[i];
		segment.generationCount = 0;
		segments_.push_back(segment);
	}
	for (int i=0; i<segments_.size(); i++)
	{
		OneSegment& segment = segments_[i];
		if (i==0)
		{
			segments_[i].T1 = segments_[i].dest;
		}else
		{
			segments_[i].T1 = segments_[i-1].T2;
		}

		if (i==segments_.size()-1)
		{
			segments_[i].T2 = segments_[i].dest;
		}else
		{
			segments_[i].T2 = segments_[i].dest + segments_[i+1].dest;
			segments_[i].T2.Normalize();
		}

		segment.p1 = segment.begin-segment.T1*(distance_);
		segment.p2 = segment.end-segment.T2*(distance_);
		Vect3f seg = segment.p2-segment.p1;
		segment.lenght2 = seg.norm();
		segment.pDir = seg.Normalize();
	}
	waveSize_ = Vect2f(40,20);
	//waveSize_ *=scaleMin_;
}

void cFixedWaves::SetPoint(int num, const Vect3f& point)
{
	if (num >= 0 && num < points_.size())
	{
		for (int i=0; i<points_.size(); i++)
		{
			if (i == num)
			{
				points_[i] = point;
				break;
			}
		}
	}
}

void cFixedWaves::DeletePoint(int num)
{
	if (num >= 0 && num < points_.size())
	{
		points_.erase(points_.begin()+num);
		selectedPoint_ = -1;
	}
}


int cFixedWaves::SelectPoint(const Vect3f &selectCoord)
{
	selectedPoint_ = -1;
	for (int i=0; i<points_.size(); i++)
	{
		Vect3f pos = points_[i];
		if (selectCoord.x > pos.x-10 && selectCoord.x < pos.x+10 &&
			selectCoord.y > pos.y-10 && selectCoord.y < pos.y+10)
		{
			selectedPoint_ = i;
			return i;
		}
	}
	return -1;
}
void cFixedWaves::serialize(Archive& ar)
{
	ar.serialize(points_, "points", 0);
	ar.serialize(textureName_, "textureName", 0);
	ar.serialize(name_, "name", 0);
	ar.serialize(distance_, "distance", 0);
	ar.serialize(sizeMin_, "sizeMin", 0);
	ar.serialize(sizeMax_, "sizeMax", 0);
	if(!ar.serialize(speed_,"speedWave", 0)){
		float speed;
		ar.serialize(scaleMin_, "scaleMin", 0);
		ar.serialize(scaleMax_, "scaleMax", 0);
		ar.serialize(speed, "speed", 0);
		speed_ = speed*100;
		sizeMin_ = 40*scaleMin_;
		sizeMax_ = 40*scaleMax_;
	}
	ar.serialize(generationTime_, "generationTime", 0);
	ar.serialize(invert_, "invert", 0);

	if(ar.isInput()){
		CreateSegments();
		SetTexture(textureName_);
		//texture_ = GetTexLibrary()->GetElement(textureName_.c_str());
	}

}

void cFixedWaves::PreDraw(cCamera *pCamera)
{
}
void cFixedWaves::Draw(cCamera *pCamera)
{
	if (points_.size() <= 1)
		return;

	//CalcWaveLines();

	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetWorldMaterial(ALPHA_BLEND,MatXf::ID, 0, texture_);
	bool avi_texture = texture_&&texture_->IsAviScaleTexture();

	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();

	sColor4f diff = pCamera->GetScene()->GetTileMap()->GetDiffuse();
	diff.r = min(diff.r+diff.a,1.0f);
	diff.g = min(diff.g+diff.a,1.0f);
	diff.b = min(diff.b+diff.a,1.0f);

	Vect3f p1,p2,p3,p4;
	float sizex = 40;
	float sizey = 20;
	for (int i=0; i<waves_.size(); i++)
	{
		if(waves_.IsFree(i))
			continue;
		cWater* water = parent_->GetWater();
		OneWave& wave = waves_[i];
		Vect3f size;
		if (parent_->IsIce(wave.pos.xi(),wave.pos.yi()) || !parent_->IsWater(wave.pos))
		{
			waves_.SetFree(i);
			continue;
		}
		if (!wave.coast)
		{
			wave.distance += wave.speed*dt;
			wave.phase = wave.distance/distance_;

		}else
		{
			wave.phase += wave.speed/50*dt;
		}

		if (wave.phase>1)
		{
			
			if (!wave.coast)
			{
				wave.phase = 0;
				wave.coast = true;
				wave.pos -= wave.dir*wave.size.y*sizeYKeys_.Get(1);
			}else
			{
				waves_.SetFree(i);
				continue;
			}
		}
		if (!wave.coast)
		{
			wave.pos += wave.dir*wave.speed*speedKeys_.Get(wave.phase)*dt;
			size.x = wave.size.x*sizeXKeys_.Get(wave.phase);
			size.y = wave.size.y*sizeYKeys_.Get(wave.phase);
			diff.a = wave.phase;
			Vect3f sx(wave.dir.y, -wave.dir.x, 0);
			Vect3f sy(wave.dir.x, wave.dir.y, 0);
			sx *= size.x;
			sy *= size.y;
			p1 = wave.pos-sx-sy;
			p2 = wave.pos-sx+sy;
			p3 = wave.pos+sx-sy;
			p4 = wave.pos+sx+sy;

		}else
		{
			size.x = wave.size.x*sizeXKeys_.Get(1);
			wave.size.y += dt*(wave.phase <= 0.5f ? wave.speed : -wave.speed/4);
			size.y = wave.size.y*sizeYKeys_.Get(1);
			diff.a = 1-wave.phase;
			Vect3f sx(wave.dir.y, -wave.dir.x, 0);
			Vect3f sy(wave.dir.x, wave.dir.y, 0);
			sx *= size.x;
			sy *= size.y;
			p1 = wave.pos-sx;
			p2 = wave.pos-sx+2*sy;
			p3 = wave.pos+sx;
			p4 = wave.pos+sx+2*sy;
		}


		
		/*		
		if (!wave.coast)
			wave.phase = wave.distance/distance_;
		else
			wave.phase += wave.speed/50*dt;

		if (wave.phase > 1)
		{
			if (!wave.coast)
			{
				wave.phase = 0;
				wave.coast = true;
			}else
			{
				waves_.SetFree(i);
				continue;
			}
		}
		Vect2f size;
		if (!wave.coast)
		{
			if (wave.phase <= 0.5f)
			{
				size = wave.size*(1+wave.phase*2);
			}else
			{
				size.x = wave.size.x*(2+(wave.phase-0.5f));
				size.y = wave.size.y*(2-(wave.phase-0.5f)*2);
			}
			wave.pos += wave.dir*wave.speed*dt;
			color.a = wave.phase*255;

		}else
		{
			if (wave.phase <= 0.5f)
			{
				wave.pos += wave.dir*wave.speed*(1-wave.phase*2)*dt;
				size.x = wave.size.x*(2.5f+wave.phase);
				size.y = wave.size.y*(1+wave.phase*3);
			}else
			{
				size.x = wave.size.x*(3.f);
				size.y = wave.size.y*(2.5f-(wave.phase-0.5f));
			}
			color.a = (1-wave.phase)*255;
		}
		//if (!wave.coast)
		//{
		//	p1 = wave.rect[0]+wave.dir*distance_*wave.phase;
		//	p2 = wave.rect[1]+wave.dir*distance_*wave.phase;
		//	p3 = wave.rect[2]+wave.dir*distance_*wave.phase;
		//	p4 = wave.rect[3]+wave.dir*distance_*wave.phase;
		//	color.a = wave.phase*255;
		//}else
		//{
		//	float s = 2*((wave.phase<0.5f) ? wave.phase : 1-wave.phase);
		//	p1 = wave.rect[0]+wave.dir*distance_+wave.dir*waveSize_.y*s/4;
		//	p2 = wave.rect[1]+wave.dir*distance_+wave.dir*waveSize_.y*s/4;
		//	p3 = wave.rect[2]+wave.dir*distance_+wave.dir*waveSize_.y*s;
		//	p4 = wave.rect[3]+wave.dir*distance_+wave.dir*waveSize_.y*s;
		//	color.a = (1-wave.phase)*255;
		//}
/**/
		p1.z = parent_->water_->GetZ(round(p1.x),round(p1.y));
		p2.z = parent_->water_->GetZ(round(p2.x),round(p2.y));
		p3.z = parent_->water_->GetZ(round(p3.x),round(p3.y));
		p4.z = parent_->water_->GetZ(round(p4.x),round(p4.y));

		const sRectangle4f& rt =  avi_texture ? ((cTextureAviScale*)texture_)->GetFramePos(wave.texturePhase) : 
															sRectangle4f::ID;	
		if (avi_texture)
		{
			wave.texturePhase += dt*textureAnimationTime_;
			if (wave.texturePhase>1.f)
				wave.texturePhase-=1.f;
		}

		sVertexXYZDT1 *v=pBuf->Get();
		v[0].pos = p1;	v[0].diffuse.set(diff); 
		v[1].pos = p2;	v[1].diffuse.set(diff); 
		v[2].pos = p3;	v[2].diffuse.set(diff); 
		v[3].pos = p4;	v[3].diffuse.set(diff); 
		v[0].GetTexel().set(rt.max.x,rt.max.y);
		v[1].GetTexel().set(rt.max.x,rt.min.y);
		v[2].GetTexel().set(rt.min.x,rt.max.y);
		v[3].GetTexel().set(rt.min.x,rt.min.y);
	}
	pBuf->EndDraw();

}
void cFixedWaves::Animate(float dt)
{
	this->dt = dt*1e-3f;
	time_ += this->dt;
	if (time_ > generationTime_)
	{
		time_ = 0;
		CalcWaveLine();
	}
}

void cFixedWaves::CalcWaveLine()
{
	if (segments_.size()<1)
		return;

	for (int i=0; i<segments_.size(); i++)
	{
		OneSegment& segment = segments_[i];
		float sx = sizeMin_+(sizeMax_-sizeMin_)/2;
		float gen = segment.lenght/sx/2;
		int count = int(gen+segment.generationCount);
		segment.generationCount += gen-(float)count;

		for (int j=0; j<count; j++)
		{
			OneWave& wave = waves_.GetFree();

			float shift = segment.lenght2/count*j+graphRnd.frnd(sx/2);
			float t = shift/segment.lenght2;

			float cosm = segment.T1.dot(segment.T2);
			Vect3f s;
			if (cosm < 0.0f)
				s = -segment.T2;
			else
				s = segment.T2;

			float scale0 = 1.0f-t;
			float scale1 = t;

			Vect3f dir;
			dir.x = scale0*segment.T1.x+scale1*s.x;
			dir.y = scale0*segment.T1.y+scale1*s.y;
			dir.z = scale0*segment.T1.z+scale1*s.z;
			//dir.x = segment.T1.x-t*(segment.T1.x+s.x);
			//dir.y = segment.T1.y-t*(segment.T1.y+s.y);
			//dir.z = segment.T1.z-t*(segment.T1.z+s.z);
			dir.Normalize();

			wave.size.x = sizeMin_+graphRnd.fabsRnd()*(sizeMax_-sizeMin_);
			wave.size.y = wave.size.x/2;
			wave.size /= 2;
			wave.pos = segment.p1 + segment.pDir*shift;
			wave.dir = dir*invert_;
			wave.coast = false;
			wave.phase = 0;
			wave.speed = speed_+graphRnd.frnd(speed_/2);
			wave.distance = 0;
			wave.texturePhase = 0;
		}
	}
}
void cFixedWaves::SetParent(cFixedWavesContainer* parent)
{
	parent_ = parent;
}
void cFixedWaves::SelectPoint(int pnt)
{
	if (points_.size() < pnt+1)
		selectedPoint_ = -1;
	else
		selectedPoint_ = pnt;
}
void cFixedWaves::SetPoint(const Vect3f& point)
{
	SetPoint(selectedPoint_,point);
}
void cFixedWaves::DeletePoint()
{
	DeletePoint(selectedPoint_);
}
void cFixedWaves::SetTexture(string textureName)
{
	RELEASE(texture_);
	textureName_ = textureName;
	if (textureName_.find(".avi") == string::npos)
		texture_ = GetTexLibrary()->GetElement2D(textureName_.c_str());
	else
	{
		texture_ = GetTexLibrary()->GetElement3DAviScale(textureName_.c_str());
		if (texture_)
			textureAnimationTime_ = ((cTextureAviScale*)texture_)->GetTotalTime()*1e-3f;
	}
}

//////////////////////////////////////////////////////////////////////////
cFixedWavesContainer::cFixedWavesContainer(cWater* water, cTemperature* temperature) : cBaseGraphObject(0)
{
	selected_ = -1;
	water_ = water;
	temperature_ = temperature;
}
bool cFixedWavesContainer::IsIce(int x,int y)
{
	if(!temperature_)
		return false;
	if (x >= (water_->GetGridSizeX()-1)<<water_->GetCoordShift() || 
		y >= (water_->GetGridSizeY()-1)<<water_->GetCoordShift() || x<0 || y<0)
		return false;
	return temperature_->checkTile(x>>temperature_->gridShift(),y>>temperature_->gridShift());
}
bool cFixedWavesContainer::IsWater(Vect3f& vec)
{
	int xx = vec.xi()>>water_->GetCoordShift();
	int yy = vec.yi()>>water_->GetCoordShift();
	if (xx<0 || xx>=water_->GetGridSizeX())
		return true;
	if (yy<0 || yy>=water_->GetGridSizeY())
		return true;
	return water_->isWater(vec);
}

cFixedWavesContainer::~cFixedWavesContainer()
{
	for (int i=0; i<listWaves_.size(); i++)
	{
		delete listWaves_[i];
	}
	listWaves_.clear();
}

cFixedWaves* cFixedWavesContainer::AddWaves()
{
	cFixedWaves* wave = new cFixedWaves();
	listWaves_.push_back(wave);
	wave->SetParent(this);
	return wave;
}

bool cFixedWavesContainer::DeleteWaves(cFixedWaves* waves)
{
	for(int i=0; i<listWaves_.size(); i++)
	{
		if (waves == listWaves_[i])
		{
			listWaves_.erase(listWaves_.begin()+i);
			delete waves;
			return true;
		}
	}
	return false;
}
void cFixedWavesContainer::ShowInfo()
{
		
	for (int i=0; i<listWaves_.size(); i++)
	{
		if (i == selected_)
			listWaves_[i]->ShowInfo(sColor4c(0,0,255));
		else
			listWaves_[i]->ShowInfo();

	}
}

cFixedWaves* cFixedWavesContainer::Select(int num)
{
	xassert(num>=0 && num<listWaves_.size());
	selected_ = num;
	return GetWave(num);
}

void cFixedWavesContainer::serialize(Archive& ar)
{
	ar.serialize(listWaves_, "listWaves_", 0);

	if (ar.isInput())
	{
		for(int i=0; i<listWaves_.size(); i++)
		{
			listWaves_[i]->SetParent(this);
		}
	}
}

void cFixedWavesContainer::PreDraw(cCamera *pCamera)
{
	pCamera->Attach(SCENENODE_OBJECTSORT,this);
}
void cFixedWavesContainer::Draw(cCamera *pCamera)
{
	start_timer_auto();
	ListWaves::iterator it;
	FOR_EACH(listWaves_,it)
	{
		cFixedWaves* w = *it;
		w->Draw(pCamera);
	}
}
void cFixedWavesContainer::Animate(float dt)
{
	start_timer_auto();
	ListWaves::iterator it;
	FOR_EACH(listWaves_,it)
	{
		cFixedWaves* w = *it;
		w->Animate(dt);
	}
}
cFixedWaves* cFixedWavesContainer::GetWave(int num)
{
	xassert(num>=0 && num<listWaves_.size());
	return listWaves_[num];
}
