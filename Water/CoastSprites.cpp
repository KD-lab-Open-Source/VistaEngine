#include "StdAfx.h"
#include "ice.h"
#include "CoastSprites.h"
#include "ResourceSelector.h"
#include "RangedWrapper.h"

// _VISTA_ENGINE_EXTERNAL_ - нужно для перевода external-версии

CoastSpriteSimpleAttributes::CoastSpriteSimpleAttributes()
{
	minSize_	= 10.f;
	maxSize_	= 18.f;
	deltaPos_	= 16.f;
	beginDeep_	= 2.f;
	endDeep_	= 3.f;
	intensity1_ = 5.f;
	intensity2_ = 5.f;
	textureName_ = "Resource\\TerrainData\\Textures\\G_Tex_Bubbles_Static_001.avi";
}

void CoastSpriteSimpleAttributes::serialize(Archive& ar)
{
	ar.serialize(minSize_, "minSize", "Минимальный размер");
	ar.serialize(maxSize_, "maxSize", "Максимальный размер");
	ar.serialize(deltaPos_, "deltaPos", "Разброс позиции появления");
	ar.serialize(beginDeep_ , "beginDeep", "Начальная глубина генерации");
	ar.serialize(endDeep_ , "endDeep", "Конечная глубина генерации");
	ar.serialize(intensity1_ , "intensity1", "Интенсивность генерации 1");
	ar.serialize(intensity2_ , "intensity2", "Интенсивность генерации 2");
	ar.serialize(ResourceSelector(textureName_, ResourceSelector::TGA_AVI_OPTIONS), "tex_name_stay", "Текстура спрайтов");
}

CoastSpriteMovingAttributes::CoastSpriteMovingAttributes()
{
	speed_		= 1.f;
	minSize_	= 1.f;
	maxSize_	= 5.f;
	deltaPos_	= 16.f;
	beginDeep_	= 3.f;
	endDeep_	= 8.f;
	intensity1_ = 20.f;
	intensity2_ = 3.f;
	textureName_ = "Resource\\TerrainData\\Textures\\G_Tex_Bubbles_Dynamic_001.avi";
}

void CoastSpriteMovingAttributes::serialize(Archive& ar)
{
	ar.serialize(speed_,"speed","Скорость Движения");
	__super::serialize(ar);
}

CoastSpritesAttributes::CoastSpritesAttributes()
{
	modeStay_			= true;
	modeMove_			= true;
	heightOverWater_	= 1.f;
	dieInCoast_			= true;
}
void CoastSpritesAttributes::serialize(Archive& ar)
{
	ar.serialize(modeStay_, "modeStay", "Показывать простые спрайты");
	ar.serialize(modeMove_, "modeMove", "Показывать движущиеся спрайты");
	ar.serialize(heightOverWater_ , "heightOverWater", "Высота над водой");
	ar.serialize(dieInCoast_ , "dieInCoast", "Исчезать на берегу");
	ar.serialize(simpleSprites_,"simpleSprites","Стоящие спрайты");
	ar.serialize(movingSprites_,"movingSprites","Движущиеся спрайты");
}

cCoastSprites::cCoastSprites(cWater* pWater_, cTemperature* pTemperature_):cBaseGraphObject(0)
{
	pSaveToAnimateCamera=NULL;
	xassert(pWater_);
	pWater = pWater_;
	pTemperature = pTemperature_;
	Texture_stay = NULL;
	Texture_mov = NULL;
	mode = CSM_NOTHING;

	dt = 0;
	add_z = 0;
	int z_shift = 16;//temp
	z_int_to_float=1.0f/(1<<z_shift);
	tex_name_stay  = "Scripts\\Resource\\balmer\\019.tga";
	tex_name_mov = "Scripts\\Resource\\balmer\\019.tga";
	simple_coast_intensity1 = move_coast_intensity1 = 8;
	simple_coast_intensity2 = move_coast_intensity2 = 1;

	grid_size = Vect2i(pWater->GetGridSizeX(), pWater->GetGridSizeY());
	grid_center = new SpriteCenterContainer[grid_size.x*grid_size.y];
	memset(grid_center, 0, sizeof(SpriteCenterContainer)*grid_size.x*grid_size.y);
	simpleMinSize = 3.0f;
	simpleMaxSize = 5.0f;
	moveMinSize = 1.0f;
	moveMaxSize = 10.0f;
	simpleDeltaPos = 1<<pWater->GetCoordShift();
	moveDeltaPos = 1<<pWater->GetCoordShift();
	speed = 0.01f;

	moveAnimationTime = 1.0f;
	simpleAnimationTime = 1.0f;

	simple_scale_time = 1.0f;
	move_scale_time = 1.0f;

	dieInCoast = true;
	soft_clamp_simple=1;
	soft_clamp_move=1;
	move_coast_dz = 1;
	move_coast_minz = 1;
	simple_coast_dz = 1;
	simple_coast_minz = 1;
}

cCoastSprites::~cCoastSprites()
{
	RELEASE(Texture_stay);
	RELEASE(Texture_mov);
	delete[] grid_center;

}
void cCoastSprites::Draw(cCamera *pCamera)
{
	start_timer_auto();
	MTAuto autolock(lock);
	DWORD old_zwrite=gb_RenderDevice->GetRenderState(RS_ZWRITEENABLE);
	gb_RenderDevice->SetRenderState(RS_ZWRITEENABLE,FALSE);
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_anisotropic);
	if (mode & CSM_SIMPLE)
		DrawSimpleCoastSprite(pCamera);
	if (mode & CSM_MOVING)
		DrawMovingCoastSprite(pCamera);
	gb_RenderDevice->SetRenderState(RS_ZWRITEENABLE,old_zwrite);
}
void cCoastSprites::PreDraw(cCamera *pCamera)
{
	pSaveToAnimateCamera=pCamera;
	if (mode != CSM_NOTHING)
		pCamera->Attach(SCENENODE_OBJECTSPECIAL,this);
}
bool cCoastSprites::IsIce(int x,int y)
{
	if(!pTemperature)
		return false;
	if (x >= (grid_size.x-1)<<pWater->GetCoordShift() || y >= (grid_size.y-1)<<pWater->GetCoordShift())
		return false;
	return pTemperature->checkTile(x>>pTemperature->gridShift(),y>>pTemperature->gridShift());
}

void cCoastSprites::SoftClampGenerate(int num_sprites,float dt,float animation_time,float& soft_clamp)
{
	const float max_num=4000;
	float out=1-clamp((num_sprites-max_num)/(max_num*0.3f),0,1.0f);
	soft_clamp=LinearInterpolate(soft_clamp,out,dt/(1.7f*animation_time));
}

void cCoastSprites::Animate(float dtime)
{
	start_timer_auto();
	if(!pSaveToAnimateCamera)
		return;
	MTAuto autolock(lock);

	cWater::OnePoint *cur = &pWater->Get(0,0);
	cWater::OnePoint *lim = cur + grid_size.x*grid_size.y;
	cWater::OnePoint *zbuffer = cur;
	SpriteCenterContainer* center = grid_center;
	int grid_shift = pWater->GetCoordShift();
	
	for (int y_ = 0; y_<grid_size.y; y_++)
	for (int x_ = 0; x_<grid_size.x; x_++, cur++, center++)
	{
		int x = x_<<grid_shift;
		int y = y_<<grid_shift;
		if (cur->z<=simple_coast_dz && cur->z>=simple_coast_minz && !IsIce(x,y))

		{
			if (!center->simpleCenter)
			{
				center->simpleCenter =  AddSpriteCenter(x,y,simple_coast_sprite_centers);
			}
		}else 
			if (center->simpleCenter)
		{
			DeleteSpriteCenter(center->simpleCenter,simple_coast_sprite_centers);
			center->simpleCenter = NULL;
		}

		if (cur->z<=move_coast_dz && cur->z>=move_coast_minz && !IsIce(x,y))
		{
			if (!center->moveCenter)
			{
				center->moveCenter =  AddSpriteCenter(x,y,move_coast_sprite_centers);
			}
		}else if (center->moveCenter)
			{
				DeleteSpriteCenter(center->moveCenter,move_coast_sprite_centers);
				center->moveCenter = NULL;
			}
	}

	cCamera *pCamera = pSaveToAnimateCamera;
	dt = dtime*1e-3f;
	ContainerCenters::iterator it;
	if (mode&CSM_SIMPLE)
	{
		SoftClampGenerate(coast_sprites.size(),dt,simpleAnimationTime,soft_clamp_simple);
		simple_scale_time = 1/simpleAnimationTime;
		FOR_EACH(simple_coast_sprite_centers,it)
		{
			cWater::OnePoint &cur = pWater->Get(it->x>>grid_shift, it->y>>grid_shift);
			float k = (cur.z-simple_coast_minz)/simple_coast_dz;
			float generate_interval = simple_min_interval*(1-k) + simple_sub_interval*k;
			it->generate_sum_static+=dt*soft_clamp_simple;
			float z = cur.realHeight()+add_z;
			if (generate_interval <= 0)
				continue;
			while(it->generate_sum_static>generate_interval)
			{
				it->generate_sum_static-=generate_interval;
				float x = it->x + graphRnd.frnd(simpleDeltaPos);
				float y = it->y + graphRnd.frnd(simpleDeltaPos);
				if (pCamera->TestVisible(x,y))
				{
					CoastSprite &p = coast_sprites.GetFree();
					p.size = graphRnd.frnd(simpleMaxSize-simpleMinSize)+simpleMinSize;//graphRnd(max_size) + min_size; 
					p.pos.set(x,y,z);
					p.phase = 0;
					p.cx = x;
					p.cy = y;
				}
			} 
		} // FOR_EACH
	} // if
	
	if (mode&CSM_MOVING)
	{
		SoftClampGenerate(mov_coast_sprites.size(),dt,moveAnimationTime,soft_clamp_move);
		move_scale_time = 1/moveAnimationTime;
		FOR_EACH(move_coast_sprite_centers, it)
		{
			Vect2f dir = pWater->GetVelocity(it->x, it->y);
			float norm = dir.norm();
			if (norm>0)
			{
				dir/=norm;
				cWater::OnePoint &cur = pWater->Get(it->x>>grid_shift, it->y>>grid_shift);
				float k = (cur.z-move_coast_minz)/move_coast_dz;
				float generate_interval = move_min_interval*(1-k) + move_sub_interval*k;
				it->generate_sum_moving+=dt*soft_clamp_move;
				float z = cur.realHeight()+add_z;
				if (generate_interval <= 0)
					continue;
				while(it->generate_sum_moving>generate_interval)
				{
					it->generate_sum_moving-=generate_interval;
					float x = it->x + graphRnd.frnd(moveDeltaPos);
					float y = it->y + graphRnd.frnd(moveDeltaPos);
					if (pCamera->TestVisible(x,y))
					{
						MovingCoastSprite& p=mov_coast_sprites.GetFree();
						p.phase = 0;
						p.pos.set(x, y, z);
						p.dir = dir;
						p.size = graphRnd(round(moveMaxSize-moveMinSize))+round(moveMinSize);
						p.cx = it->x;
						p.cy = it->y;
						Vect2f vel = pWater->GetVelocity(round(x), round(y));
						float avel = vel.norm();
						if (avel>1e-5f)
							p.dir = vel/avel;
						p.speed_x = vel.x*speed;
						p.speed_y = vel.y*speed;
					}
				} 
			} 
		} //FOR_EACH
	} // if	
}

sColor4c cCoastSprites::GetDiffuseColor(cCamera *pCamera)
{
	sColor4f diff = pCamera->GetScene()->GetTileMap()->GetDiffuse();
	diff.r = min(diff.r+diff.a,1.0f);
	diff.g = min(diff.g+diff.a,1.0f);
	diff.b = min(diff.b+diff.a,1.0f);
	return sColor4c(diff);
}
	
#define INT_SIZE 65535
#define INT_SIZE_HALF 32768

void cCoastSprites::DrawSimpleCoastSprite(cCamera *pCamera)
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetWorldMaterial(ALPHA_BLEND,MatXf::ID, 0, Texture_stay,0,COLOR_MOD,true);
	bool avi_texture = Texture_stay&&Texture_stay->IsAviScaleTexture();
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	int grid_shift = pWater->GetCoordShift();
	int phase_step = round(dt*simple_scale_time*INT_SIZE);
	sColor4c diff = GetDiffuseColor(pCamera);

	int size = coast_sprites.size(); 
	pBuf->BeginDraw();
	for(int icoast_sprites=0;icoast_sprites<size;icoast_sprites++)
	{
		if(coast_sprites.IsFree(icoast_sprites))
			continue;
		CoastSprite& s=coast_sprites[icoast_sprites];

		if(s.phase>=INT_SIZE)
		{
			coast_sprites.SetFree(icoast_sprites);
			continue;
		}

		if (dieInCoast && pWater->Get(s.cx >>grid_shift, s.cy>>grid_shift).z <= 0)
		{
			coast_sprites.SetFree(icoast_sprites);
			continue;
		}

		//if (pCamera->TestVisible(s.cx,s.cy))
		{
			int alpha=2*(((s.phase<INT_SIZE_HALF) ? (s.phase>>8) : 255-(s.phase>>8) ));
			xassert(alpha>=0 && alpha<256);
			diff.a = alpha;
			sVertexXYZDT1 *v=pBuf->Get();
			
			const sRectangle4f& rt =  avi_texture ? ((cTextureAviScale*)Texture_stay)->GetFramePosInt(s.phase) : 
																sRectangle4f::ID;	
					//p.pos[0].set(x-size, y+size, z);
					//p.pos[1].set(x+size, y+size, z);
					//p.pos[2].set(x-size, y-size, z);
					//p.pos[3].set(x+size, y-size, z);
			v[0].pos.set(s.pos.x-s.size,s.pos.y+s.size,s.pos.z); v[0].diffuse=diff; v[0].GetTexel().set(rt.min.x,rt.min.y);
			v[1].pos.set(s.pos.x+s.size,s.pos.y+s.size,s.pos.z); v[1].diffuse=diff; v[1].GetTexel().set(rt.min.x,rt.max.y);
			v[2].pos.set(s.pos.x-s.size,s.pos.y-s.size,s.pos.z); v[2].diffuse=diff; v[2].GetTexel().set(rt.max.x,rt.min.y);
			v[3].pos.set(s.pos.x+s.size,s.pos.y-s.size,s.pos.z); v[3].diffuse=diff; v[3].GetTexel().set(rt.max.x,rt.max.y);
		}
		s.phase+=phase_step;
	}
	pBuf->EndDraw();
	coast_sprites.Compress();
}
void cCoastSprites::DrawMovingCoastSprite(cCamera *pCamera)
{
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetWorldMaterial(ALPHA_BLEND,MatXf::ID, 0, Texture_mov,0,COLOR_MOD,true);
	//rd->SetWorldMaterial(ALPHA_BLEND,MatXf::ID, 0, NULL);
	bool avi_texture = Texture_mov&&Texture_mov->IsAviScaleTexture();
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	int grid_shift = pWater->GetCoordShift();
	int phase_step = round(dt*move_scale_time*INT_SIZE);
	sColor4c diff = GetDiffuseColor(pCamera);
	
	int size = mov_coast_sprites.size();

	for(int isprite=0;isprite<size;isprite++)
	{
		if(mov_coast_sprites.IsFree(isprite))
			continue;
		MovingCoastSprite& s=mov_coast_sprites[isprite];

		if(s.phase>INT_SIZE)
		{
			mov_coast_sprites.SetFree(isprite);
			continue;
		}

 		if (dieInCoast && pWater->Get(s.cx>>grid_shift, s.cy>>grid_shift).z <= 0)
		{
			mov_coast_sprites.SetFree(isprite);
			continue;
		}

		s.pos.x+=s.speed_x;
		s.pos.y+=s.speed_y;
		//if (pCamera->TestVisible(s.cx,s.cy))
		{ 
			int alpha=2*(((s.phase<INT_SIZE_HALF) ? (s.phase>>8) : 255-(s.phase>>8) ));
			xassert(alpha>=0 && alpha<256);
			diff.a = alpha;
			float sx_x = s.dir.y;
			float sx_y = -s.dir.x;

			float sy_x = s.dir.x;
			float sy_y = s.dir.y;

			sx_x*= s.size;
			sx_y*= s.size;
			
			sy_x*= s.size;
			sy_y*= s.size;
		
			sVertexXYZDT1 *v=pBuf->Get();
			const sRectangle4f& rt =  avi_texture ? ((cTextureAviScale*)Texture_mov)->GetFramePosInt(s.phase): 
																sRectangle4f::ID;	
/*
			-s.dir.y-s.dir.x, s.dir.x-s.dir.y
			-s.dir.y+s.dir.x, s.dir.x+s.dir.y
			 s.dir.y-s.dir.x,-s.dir.x-s.dir.y 
			 s.dir.y+s.dir.x,-s.dir.x+s.dir.y

			 xpy=s.dir.x+s.dir.y,xmy=s.dir.x-s.dir.y
			-xpy, xmy
			 xmy, xpy
			-xmy,-xpy
			 xpy,-xmy
		    //Преобразовать к такому виду и учитывать aspect ratio у спрайтика.
*/

			v[0].pos.set(s.pos.x-sx_x-sy_x,s.pos.y-sx_y-sy_y,s.pos.z);	v[0].diffuse=diff; v[0].GetTexel().set(rt.min.x,rt.min.y);	
			v[1].pos.set(s.pos.x-sx_x+sy_x,s.pos.y-sx_y+sy_y,s.pos.z);	v[1].diffuse=diff; v[1].GetTexel().set(rt.min.x,rt.max.y);
			v[2].pos.set(s.pos.x+sx_x-sy_x,s.pos.y+sx_y-sy_y,s.pos.z);	v[2].diffuse=diff; v[2].GetTexel().set(rt.max.x,rt.min.y);	
			v[3].pos.set(s.pos.x+sx_x+sy_x,s.pos.y+sx_y+sy_y,s.pos.z);	v[3].diffuse=diff; v[3].GetTexel().set(rt.max.x,rt.max.y);
		}
		s.phase+=phase_step;
	}
	pBuf->EndDraw();
	mov_coast_sprites.Compress();
}


SpriteCenter* cCoastSprites::AddSpriteCenter(int x, int y,ContainerCenters &container)
{
	SpriteCenter t;
	t.x = x;
	t.y = y;
	t.generate_sum_static = graphRnd.frand()-0.5f;
	t.generate_sum_moving = graphRnd.frand()-0.5f;
	//t.mode = 0;
	container.push_front(t);
	return &container.front();
}

void cCoastSprites::DeleteSpriteCenter(SpriteCenter* sprite_center,ContainerCenters &container)
{
	ContainerCenters::iterator it;
	FOR_EACH(container, it)
		if (sprite_center==&(*it))
		{
			//it->mode = 2;
			//it->generate_sum_static =
			//it->generate_sum_moving = 1;
			container.erase(it);
			break;
		}
}

void cCoastSprites::SetSpriteParameters(const CoastSpriteSimpleAttributes* attributes, bool simple)
{
	if(!attributes)
		return;

	MTAuto autolock(lock);

	if (simple)
	{
		simple_coast_minz = round(attributes->beginDeep_/z_int_to_float);
		simple_coast_dz = round(attributes->endDeep_/z_int_to_float);// - deep1;
		simple_min_interval = 1/attributes->intensity1_;
		simple_sub_interval = 1/attributes->intensity2_;// - 1/intensity1;
		
		if (simple_coast_minz==0) simple_coast_minz = 1;
		if (simple_coast_dz<simple_coast_minz)simple_coast_dz = simple_coast_minz;

		simpleDeltaPos = attributes->deltaPos_;
		simpleMinSize = attributes->minSize_;
		simpleMaxSize = attributes->maxSize_;

		RELEASE(Texture_stay);
		if (!attributes->textureName_.empty() && strstr(attributes->textureName_.c_str(), ".avi") )
		{
			Texture_stay = GetTexLibrary()->GetElement3DAviScale(attributes->textureName_.c_str());
			simpleAnimationTime = ((cTextureAviScale*)Texture_stay)->GetTotalTime()*1e-3f;
		}
		else 
			Texture_stay = GetTexLibrary()->GetElement3D(attributes->textureName_.c_str());
	}else
	{
		move_coast_minz = round(attributes->beginDeep_/z_int_to_float);
		move_coast_dz = round(attributes->endDeep_/z_int_to_float);// - deep1;
		move_min_interval = 1/attributes->intensity1_;
		move_sub_interval = 1/attributes->intensity2_;// - 1/intensity1;
		
		if (move_coast_minz==0) move_coast_minz = 1;
		if (move_coast_dz<move_coast_minz)move_coast_dz = move_coast_minz;

		moveDeltaPos = attributes->deltaPos_;
		moveMinSize = attributes->minSize_;
		moveMaxSize = attributes->maxSize_;

		RELEASE(Texture_mov);
		if (!attributes->textureName_.empty() && strstr(attributes->textureName_.c_str(), ".avi") )
		{
			Texture_mov = GetTexLibrary()->GetElement3DAviScale(attributes->textureName_.c_str());
			moveAnimationTime = ((cTextureAviScale*)Texture_mov)->GetTotalTime()*1e-3f;
		}
		else 
			Texture_mov = GetTexLibrary()->GetElement3D(attributes->textureName_.c_str());
	}

	coast_sprites.clear();
	mov_coast_sprites.clear();
}
CoastSpritesMode cCoastSprites::GetMode()
{
	return mode;
}
void cCoastSprites::SetMode(CoastSpritesMode m)
{
	MTAuto autolock(lock);
	if (mode&CSM_MOVING && !(m&CSM_MOVING))
		mov_coast_sprites.clear();
	if (mode&CSM_SIMPLE && !(m&CSM_SIMPLE))
		coast_sprites.clear();
	mode = m;
}


void cCoastSprites::Init(const CoastSpritesAttributes& attributes)
{
	add_z = attributes.heightOverWater_;
	dieInCoast = attributes.dieInCoast_;
	if (attributes.modeStay_ || attributes.modeMove_)
	{
		CoastSpritesMode m = CSM_NOTHING;
		if (attributes.modeStay_)	m= CoastSpritesMode(m|CSM_SIMPLE);
		if (attributes.modeMove_)   m= CoastSpritesMode(m|CSM_MOVING);
		SetMode(m);
		speed = attributes.movingSprites_.speed_/100;

		SetSpriteParameters(&attributes.simpleSprites_);
		SetSpriteParameters(&attributes.movingSprites_,false);

	}else
	{
		SetMode(CSM_NOTHING);
		simple_coast_sprite_centers.clear();
		move_coast_sprite_centers.clear();
		coast_sprites.clear();
		mov_coast_sprites.clear();
		Vect2i grid_size(pWater->GetGridSizeX(), pWater->GetGridSizeY());
		memset(grid_center, 0, sizeof(SpriteCenterContainer)*grid_size.x*grid_size.y);
	}

}

/*
void cCoastSprites::serialize(Archive& ar)
{
//	string sname = buble_spume_texture_name;

	CoastSpritesMode mode = CSM_NOTHING;
	mode = GetMode();
	float simple_deep1 = simple_spume_deep1*z_int_to_float;
	float simple_deep2 = simple_spume_deep2*z_int_to_float;
	float simple_intensity1 = simple_coast_intensity1;
	float simple_intensity2 = simple_coast_intensity2;
	float move_deep1 = move_spume_deep1*z_int_to_float;
	float move_deep2 = move_spume_deep2*z_int_to_float;
	float move_intensity1 = move_coast_intensity1;
	float move_intensity2 = move_coast_intensity2;
	float _speed = speed*100;

	bool mode_stay = GetMode()&CSM_SIMPLE;
	bool mode_mov = GetMode()&CSM_MOVING;

	ar.openBlock("Coast_Sprites", "Прибрежные спрайты");
		ar.serialize(mode_stay, "need_stay", "Показывать простые спрайты");
		ar.serialize(mode_mov, "need_moving", "Показывать движущиеся спрайты");
		ar.serialize(add_z , "buble_add_z", "Высота над водой");
		ar.serialize(dieInCoast , "dieInCoast", "Исчезать на берегу");
		ar.openBlock("Standing Sprites","Стоящие спрайты");
			ar.serialize(simpleMinSize, "simpleMinSize", "Минимальный размер");
			ar.serialize(simpleMaxSize, "simpleMaxSize", "Максимальный размер");
			ar.serialize(simpleDeltaPos, "simpleDeltaPos", "Разброс позиции появления");
			ar.serialize(simple_deep1 , "simple_buble_deep1", "Начальная глубина генерации");
			ar.serialize(simple_deep2 , "simple_buble_deep2", "Конечная глубина генерации");
			ar.serialize(simple_intensity1 , "simple_buble_intensity1", "Интенсивность генерации 1");
			ar.serialize(simple_intensity2 , "simple_buble_intensity2", "Интенсивность генерации 2");
			ar.serialize(ResourceSelector(tex_name_stay, ResourceSelector::TGA_AVI_OPTIONS), "tex_name_stay", "Текстура для стоящих спрайтов");
		ar.closeBlock();
		ar.openBlock("Moving Sprites","Движущиеся спрайты");
			ar.serialize(_speed, "speed", "Скорость Движения");
			ar.serialize(moveMinSize, "moveMinSize", "Минимальный размер");
			ar.serialize(moveMaxSize, "moveMaxSize", "Максимальный размер");
			ar.serialize(moveDeltaPos, "moveDeltaPos", "Разброс позиции появления");
			ar.serialize(move_deep1 , "move_buble_deep1", "Начальная глубина генерации");
			ar.serialize(move_deep2 , "move_buble_deep2", "Конечная глубина генерации");
			ar.serialize(move_intensity1 , "move_buble_intensity1", "Интенсивность генерации 1");
			ar.serialize(move_intensity2 , "move_buble_intensity2", "Интенсивность генерации 2");
			ar.serialize(ResourceSelector(tex_name_mov, ResourceSelector::TGA_AVI_OPTIONS), "tex_name_mov", "Текстура для движущихся спрайтов");
		ar.closeBlock();
	ar.closeBlock();
	if (ar.isInput())
	{
		if (mode_stay || mode_mov)
		{
			CoastSpritesMode m = CSM_NOTHING;
			if (mode_stay)	m= CoastSpritesMode(m|CSM_SIMPLE);
			if (mode_mov)   m= CoastSpritesMode(m|CSM_MOVING);
			SetMode(m);
			simple_spume_deep1 = round(simple_deep1/z_int_to_float);
			simple_spume_deep2 = round(simple_deep2/z_int_to_float);
			simple_coast_intensity1 = simple_intensity1;
			simple_coast_intensity2 = simple_intensity2;

			move_spume_deep1 = round(move_deep1/z_int_to_float);
			move_spume_deep2 = round(move_deep2/z_int_to_float);
			move_coast_intensity1 = move_intensity1;
			move_coast_intensity2 = move_intensity2;
			speed = _speed/100;
			
			SetSpriteParameters(simple_spume_deep1, simple_spume_deep2, simple_intensity1, simple_intensity2);
			//SetSpriteParameters(simple_spume_deep1, simple_spume_deep2, simple_intensity1, simple_intensity2,false);
			SetSpriteParameters(move_spume_deep1, move_spume_deep2, move_intensity1, move_intensity2,false);
			
			if (simple_spume_deep1==0) simple_spume_deep1 = 1;
			if (simple_spume_deep2<simple_spume_deep1)simple_spume_deep2 = simple_spume_deep1;
			
			if (move_spume_deep1==0) move_spume_deep1 = 1;
			if (move_spume_deep2<move_spume_deep1)move_spume_deep2 = move_spume_deep1;

			if (simpleDeltaTime > simpleScaleTime) simpleDeltaTime = simpleScaleTime;
			if (moveDeltaTime > moveScaleTime) moveDeltaTime = moveScaleTime;
		}else
		{
			SetMode(CSM_NOTHING);
			simple_coast_sprite_centers.clear();
			move_coast_sprite_centers.clear();
			coast_sprites.clear();
			mov_coast_sprites.clear();
			Vect2i grid_size(pWater->GetGridSizeX(), pWater->GetGridSizeY());
			memset(grid_center, 0, sizeof(SpriteCenterContainer)*grid_size.x*grid_size.y);
		}
	}

}
	*/