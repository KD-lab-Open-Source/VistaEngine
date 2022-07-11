#include "StdAfx.h"
#include "FallLeaves.h"
#include "Terra\vmap.h"
#include "Physics\WindMap.h"
#include "Serialization\ResourceSelector.h"
#include "FileUtils\FileUtils.h"
#include "Environment\Environment.h"
#include "SkyObject.h"
#include "FileUtils\FileUtils.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\TileMap.h"
#include "Render\Src\TexLibrary.h"
#include "Render\Src\Scene.h"

/*
  О листопаде.
  1) Необходимо регистрировать и удалять объекты (с lock для многопоточности).
  2) Только видимые на экране объекты должны генерировать листья.
  3) Листья падают из треугольников ближе к вершине дерева.
*/

cFallLeaves::cFallLeaves()
: BaseGraphObject(0)
, pTexture(0)
, deltatime(0.0f)

, intensity_(10.0f)
, intensityBlow_(200.0f)
, intensityWindMult_(1.0f)
, rotationSpeedMax_(15.0f)

{
	initTextures();
}

cFallLeaves::~cFallLeaves()
{
	RELEASE(pTexture);
}

void cFallLeaves::updateObjectTexture(OneObject& object)
{
    std::string fileName = normalizePath(object.model->GetFileName());
    
	ModelToTextureIndexMap::iterator it = modelToTextureIndex_.find(fileName.c_str());
	if(it == modelToTextureIndex_.end())
		textureAttributes_[0].push_back(fileName.c_str());

	int index = modelToTextureIndex_[fileName.c_str()];
	object.texture_index = index;
	object.scale = textureAttributes_[index].scale;
	object.intensity = intensity_ * textureAttributes_[index].intensity;
}


HandleFallLeaves cFallLeaves::createHandle(c3dx* model, float intensityScale)
{
	MTAuto lock(objects_lock);
	
	HandleFallLeaves handle;
	handle.data = objects.GetIndexFree();
	OneObject& object = objects[handle.data];

	object.scale = 1.0f;
	object.model = model;
	object.intensity_remainder = 0;
	object.intensity_blow = 0.0f;

	updateObjectTexture(object);

	return handle;
}

void cFallLeaves::deleteHandle(HandleFallLeaves& handle)
{
	if(handle.data == -1)
		return;
	MTAuto lock(objects_lock);
	objects.SetFree(handle.data);
	handle.data=-1;
}

void cFallLeaves::Animate(float dt)
{
	deltatime=dt*1e-3f;
} 

void cFallLeaves::PreDraw(Camera* camera)
{
	start_timer_auto();
	camera->Attach(SCENENODE_OBJECTSORT,this);
	AnimateObjects(camera);
	AnimateParticles();
}

void cFallLeaves::Draw(Camera* camera)
{
	start_timer_auto();

	if(!pTexture || pTexture->GetFramesCount() == 0 || !scene()->GetTileMap())
		return;
	
	MTAuto lock(objects_lock);
	gb_RenderDevice->SetWorldMaterial(ALPHA_BLEND,MatXf::ID,0,pTexture);

	Color4c tileMapColor(scene()->GetTileMap()->GetDiffuse());
	Color4c sunColor(environment->environmentTime()->GetCurSunColor());
	Color4c color(tileMapColor);

	Vect3f lightDirection = Vect3f::ZERO - environment->environmentTime()->sunPosition();
	lightDirection.normalize();

	Mat3f mat=camera->GetMatrix().rot();
	cQuadBuffer<sVertexXYZDT1>* pBuf=gb_RenderDevice->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	
	int texturesCount = pTexture->GetFramesCount();
	std::vector<sRectangle4f> rects;
	rects.resize(texturesCount);
	
	for(int i = 0; i < texturesCount; ++i)
		rects[i] = pTexture->GetFramePos(i);
	
	int size=particles.size();
	for(int index=0;index<size;index++)
	{
		if(particles.IsFree(index))
			continue;
		OneParticle& p=particles[index];
		sRectangle4f rt(rects[p.texture_index]);


		Vect3f& pos=p.pos;
		Vect2f rot(1,0);
		Vect3f sx,sy;
		mat=Mat3f(p.z_angle,Z_AXIS)*Mat3f(p.rotate_angle,X_AXIS);
		mat.xform(Vect3f(rot.x * p.scale,-rot.y * p.scale,0),sx);
		mat.xform(Vect3f(rot.y * p.scale,rot.x * p.scale,0),sy);
		//color.interpolate(tileMapColor, sunColor, max(mat.zrow().dot(lightDirection), (-mat.zrow()).dot(lightDirection)));

		color.a=round(p.alpha*255);
		
		sVertexXYZDT1 *v=pBuf->Get();
		v[0].pos.x=pos.x-sx.x-sy.x;	v[0].pos.y=pos.y-sx.y-sy.y;	v[0].pos.z=pos.z-sx.z-sy.z;
		v[0].diffuse=color;
		v[0].GetTexel().x = rt.min.x; v[0].GetTexel().y = rt.min.y;	//	(0,0);

		v[1].pos.x=pos.x-sx.x+sy.x;	v[1].pos.y=pos.y-sx.y+sy.y;	v[1].pos.z=pos.z-sx.z+sy.z;
		v[1].diffuse=color;
		v[1].GetTexel().x = rt.min.x; v[1].GetTexel().y = rt.max.y;//	(0,1);
		
		v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; 
		v[2].diffuse=color;
		v[2].GetTexel().x = rt.max.x; v[2].GetTexel().y = rt.min.y;	//  (1,0);
		
		v[3].pos.x=pos.x+sx.x+sy.x;	v[3].pos.y=pos.y+sx.y+sy.y;	v[3].pos.z=pos.z+sx.z+sy.z;
		v[3].diffuse=color;
		v[3].GetTexel().x = rt.max.x;v[3].GetTexel().y = rt.max.y;//  (1,1);
	}
	pBuf->EndDraw();
}
/*
((!a && b) && c) || (a && b)

a b c
f f f - f
t f f - f
f t f - f
t t f - t
f f t - f
t f t - f
f t t - t
t t t - t

b && (c || a)
*/

void cFallLeaves::AnimateObjects(Camera* camera)
{
	MTAuto lock(objects_lock);
	//Пока деревьев меньше 2000-3000, выгоднее по всем пробежаться, с быстрой проверкой видимости.
	int size=objects.size();
	for(int index=0;index<size;index++)
	{
		if(objects.IsFree(index))
			continue;
		OneObject& p=objects[index];
		const Vect2f& posf=p.model->GetPosition().trans();
		int x=round(posf.x),y=round(posf.y);
		if(!camera->TestVisible(x,y))
			continue;

		Vect2f wind_speed = windMap->getPerlin(posf);
		float wind_mult = clamp(wind_speed.norm() * intensityWindMult_, 0.01f, 20.0f);

		p.intensity_blow = max(0.0f, p.intensity_blow - deltatime);
		p.intensity_remainder += (p.intensity + wind_mult + intensityBlow_ * p.intensity_blow) * deltatime;

		int num_point = round(p.intensity_remainder);
		p.intensity_remainder -= float(num_point);

		if(num_point){
			vector<Vect3f> pos(max(num_point, 4));
			vector<Vect3f> norm;
			p.model->GetVisibilityVertex(pos, norm);

			for(int i = 0; i < num_point; i++){
				OneParticle& one = particles.GetFree();

				Vect3f p1 = pos[i];
				Vect3f p2 = pos[xm_random_generator() % pos.size()];

				float f = xm_random_generator.frand();
				one.texture_index = p.texture_index;
				one.pos=p1*f+p2*(1-f);
				one.scale=p.scale;
				one.state=P_BEGIN_FALL;
				one.alpha=0;
				one.z_angle=xm_random_generator.frand()*2*M_PI;
				one.rotate_angle=xm_random_generator.frand()*2*M_PI;
				one.rotate_speed = rotationSpeedMax_ *xm_random_generator.frand();
			}
		}
	}
}

/*
   О падении лисьев -
     листья равномерно вращаются вокруг определённой оси,
	 а перемещаются перпендикулярно оси вращения.

   Когда лист - параллельно земле - он восновном в сторону летит, 
						когда перпендикулярно - вниз.
					
*/

void cFallLeaves::AnimateParticles()
{
	float speed_down=-20;
	float wind_k=10;
	float alpha_speed=1;

	int size=particles.size();
	for(int index=0;index<size;index++)
	{
		if(particles.IsFree(index))
			continue;
		OneParticle& p=particles[index];


		Vect2f wind_speed = windMap->getPerlin(p.pos);
		if(p.state == P_BEGIN_FALL){
			p.alpha+=alpha_speed*deltatime;
			if(p.alpha>=1)
			{
				p.alpha=1;
				p.state=P_FALL;
			}
		}

		
		if(p.state != P_END_FALL){
			p.rotate_angle+=p.rotate_speed*deltatime;
			p.pos.z+=speed_down*deltatime;
			p.pos.x+=wind_speed.x*wind_k*deltatime;
			p.pos.y+=wind_speed.y*wind_k*deltatime;
			int x = round(p.pos.x);
			int y = round(p.pos.y);
			int z = (vMap.getAltC(x,y) >> VX_FRACTION) + 2;
			if(round(p.pos.z) < z){
				p.pos.z=z;
				p.state=P_END_FALL;
				continue;
			}
		}
		else{
			p.alpha-=alpha_speed*deltatime;
			if(p.alpha<=0)
			{
				p.alpha=0;
				particles.SetFree(index);
			}
		}

	}
	particles.Compress();
}

void cFallLeaves::serializeForModel(Archive& ar, const char* modelName)
{
	std::string modelFileName = normalizePath(modelName);
	std::string comboList;

	TextureAttributes::const_iterator it;
	FOR_EACH(textureAttributes_, it){
		if(it != textureAttributes_.begin())
			comboList += "|";
		comboList += ::extractFileName(it->texture.c_str());
	}

	xassert(!textureAttributes_.empty());

	ModelToTextureIndexMap::const_iterator i = modelToTextureIndex_.find(modelFileName);
	int index = (i == modelToTextureIndex_.end()) ? 0 : i->second;
	std::string value = extractFileName(textureAttributes_.at(index).texture.c_str());
	
	ComboListString fallingTexture(comboList.c_str(), value.c_str());

	ar.serialize(fallingTexture, "fallingLeavesTexture", "Текстура листьев (для всех моделей)"); 

	if(ar.isInput()){
		int textureIndex = indexInComboListString(fallingTexture.comboList(), fallingTexture.value().c_str());
		if(modelToTextureIndex_[modelFileName] == textureIndex)
			return;

		TextureAttributes::iterator it;
		FOR_EACH(textureAttributes_, it){
			TextureAttribute::iterator mit = std::find(it->begin(), it->end(), modelFileName.c_str());
			if(mit != it->end())
				it->erase(mit);
		}
		xassert(textureIndex >= 0 && textureIndex < textureAttributes_.size());
		textureAttributes_[textureIndex].push_back(modelFileName.c_str());
		modelToTextureIndex_[modelFileName] = textureIndex;

		initTextures();
		int objectsSize = objects.size();
		for(int i = 0; i < objectsSize; ++i)
			if(!objects.IsFree(i))
				updateObjectTexture(objects[i]);
	}
}

void cFallLeaves::serialize(Archive& ar)
{
	ar.serialize(intensity_, "intensity", "Минимальная интенсивность");
	ar.serialize(intensityWindMult_, "intensityWindMult", "Коэф. интенсивности от ветра");
	ar.serialize(intensityBlow_, "intensityBlow", "Интенсивность падающего дерева");
	
	ar.serialize(rotationSpeedMax_, "rotationSpeedMax", "Максимальная скорость вращения");

    ar.serialize(textureAttributes_, "textureModelList", "Текстуры");

	if(ar.isInput()){
		TextureAttributes::iterator it;
		
        modelToTextureIndex_.clear();
	
		int textureIndex = 0;
		FOR_EACH(textureAttributes_, it){
			std::string texture = it->texture;
			
			TextureAttribute::const_iterator mit;
			FOR_EACH(*it, mit)
                modelToTextureIndex_[*mit] = textureIndex;
			++textureIndex;
		}

		initTextures();

		int objectsSize = objects.size();
		for(int i = 0; i < objectsSize; ++i)
			if(!objects.IsFree(i)){
				updateObjectTexture(objects[i]);
			}
	}
}

void cFallLeaves::initTextures()
{
	if(textureAttributes_.empty())
		textureAttributes_.push_back("Scripts\\Resource\\balmer\\LeafBrown.tga");
	RELEASE(pTexture);

	std::vector<std::string> textureNames;
	for(int i = 0; i < textureAttributes_.size(); ++i)
		textureNames.push_back(textureAttributes_[i].texture);
	pTexture = (cTextureComplex*)GetTexLibrary()->GetElement3DComplex(textureNames);
	xassert(pTexture);
}

void cFallLeaves::TextureAttribute::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.tga", ".\\RESOURCE\\TerrainData\\Textures", "");
	ar.serialize(ResourceSelector(texture, options), "texture", "&Текстура");
	ar.serialize(scale, "scale", "Масштаб");
	ar.serialize(intensity, "intensity", "Коэф. интенсивности");
	ar.serialize(static_cast<std::vector<std::string>&>(*this), "models", "Назначать на модели");
	if(ar.isInput() && ar.isEdit()){
		std::string copyOfString = normalizePath(texture.c_str());
		iterator it;
		FOR_EACH(*this, it)
			*it = normalizePath(it->c_str());
	}
}

void fCommandBlowFallLeaves(XBuffer& buffer)
{
	xassert(environment->fallLeaves());
	HandleFallLeaves handle;
	buffer.read(handle);

	environment->fallLeaves()->setBlow(handle);
}

void cFallLeaves::setBlow(HandleFallLeaves& handle)
{
	if(handle.isValid()){
		int index = handle.data;
		if(index >= 0 && index < objects.size()){
			OneObject& object = objects[index];
			object.intensity_blow = 1.0f;
		}
	}
}
