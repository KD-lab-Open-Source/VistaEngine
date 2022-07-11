#include "StdAfxRD.h"
#include "TileMap.h"
#include "D3DRender.h"
#include "cCamera.h"
#include "Scene.h"
#include "MultiRegion.h"
#include "VisGeneric.h"
#include "Terra\vmap.h"
#include "D3DRenderTilemap.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\RangedWrapper.h"
#include "Render\src\FileImage.h"
#include "Serialization\EnumDescriptor.h"
#include "Environment\Environment.h"
#include "FileUtils\FileUtils.h"

namespace {
ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
}

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(cTileMap, ShaderType, "ShaderType")
REGISTER_ENUM_ENCLOSED(cTileMap, LAVA, "Лава")
REGISTER_ENUM_ENCLOSED(cTileMap, ICE, "Лед")
END_ENUM_DESCRIPTOR_ENCLOSED(cTileMap, ShaderType)

cTileMap* tileMap;

cTileMap::cTileMap(cScene* pScene, bool _tryColorEnable) : BaseGraphObject(0)
{
	tileMap = this;

	tile_scale.set(1,1,1);
	setAttribute(ATTRCAMERA_REFLECTION);
	tileSize_.set(TILEMAP_SIZE,TILEMAP_SIZE);
	tileNumber_.set(0,0);
	tiles_=0;

	tileMapRender_=0;

	zMax_ = 0;

	animationTime_ = 0;
	miniDetailTextureResolutionPower_ = 4;

	enable_debug_rect=false;
	debug_fade_interval=500;

	float c=0.8f;
	SetDiffuse(Color4f(c,c,c,1-c));

	for(int i=0;i < miniDetailTexturesNumber;i++)
		zeroplast_color[i].set(1,1,1,1);

	gb_RenderDevice3D->tilemap_inv_size.x=1.0f/vMap.H_SIZE;
	gb_RenderDevice3D->tilemap_inv_size.y=1.0f/vMap.V_SIZE;
	update_zminmmax_time=1;

	setScene(pScene);

	trueColorEnable_=_tryColorEnable;

	lavaShader_ = new ShaderSceneWaterLava(true);
	lavaShader_->Restore();
	iceShader_ = new ShaderSceneWaterIce;
	iceShader_->Restore();

	vMap.registerUpdateMapClient(this);

	Vect2i size((int)vMap.H_SIZE, (int)vMap.V_SIZE);
	tileNumber_.set(size.x/tileSize().x,size.y/tileSize().y);
	xassert(tileNumber_.x*tileSize().x==size.x);
	xassert(tileNumber_.y*tileSize().y==size.y);

	tiles_ = new sTile[tileNumber().x*tileNumber().y];

	tileBordersX_ = new short[tileBordersSize_.x = tileNumber().x*size.y];
	tileBordersY_ = new short[tileBordersSize_.y = tileNumber().y*size.x];
	tileBordersShr_.set(vMap.V_SIZE_POWER - TILEMAP_SHL, vMap.H_SIZE_POWER - TILEMAP_SHL);
	heightFractionInv_ = 1.0f/float(1<<VX_FRACTION);

	tileMapRender_ = new cTileMapRender(this);

	updateMap(Vect2i(0,0), Vect2i(size.x-1,size.y-1));
}

cTileMap::~cTileMap()
{
	vMap.unregisterUpdateMapClient(this);

	delete lavaShader_;
	delete iceShader_;

	delete tileMapRender_;

	if(tiles_){
		delete[] tiles_; 
		delete[] tileBordersX_;
		delete[] tileBordersY_;
	}

	tileMap = 0;
}

STARFORCE_API void cTileMap::serialize(Archive& ar)
{
	if(ar.filter(SERIALIZE_WORLD_DATA)){
		ar.serialize(RangedWrapperi(miniDetailTextureResolutionPower_, 1, 5), "miniDetailTextureResolution", "Разрешение мелкодетальной текстуры");
		MiniDetailTexture::resolution = miniDetailTextureResolution();
		ar.serializeArray(miniDetailTextures_, "miniDetailTextureArray", 0);
	}
	if(ar.filter(SERIALIZE_GLOBAL_DATA)){
		ar.serializeArray(placementZoneMaterials_, "placementZoneMaterials", "Материалы зон установки");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cIUnkObj
//////////////////////////////////////////////////////////////////////////////////////////

void cTileMap::PreDraw(Camera* camera)
{
	start_timer_auto();

	if(getAttribute(ATTRUNKOBJ_IGNORE) || debugShowSwitch.tilemap) 
		return;

	BuildRegionPoint();

	camera->Attach(SCENENODE_OBJECT_TILEMAP,this);
	tileMapRender_->PreDraw(camera);
}

void cTileMap::Draw(Camera* camera)
{
	if(!Option_ShowType[SHOW_TILEMAP])
		return;
	start_timer_auto();

//	cD3DRender *Render=gb_RenderDevice3D;
//	if(camera->getAttribute(ATTRCAMERA_SHADOW)){
//		Render->Draw(scene()); // рисовать источники света
//	}
//	else 
	if(camera->getAttribute(ATTRCAMERA_SHADOWMAP)){
		if(Option_shadowEnabled)
			tileMapRender_->DrawBump(camera, ALPHA_TEST, true, false);
	}
	else if(camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
		tileMapRender_->DrawBump(camera, ALPHA_NONE, false, true);
	else
		tileMapRender_->DrawBump(camera, ALPHA_NONE, false, false);

	DrawLines();
}

//////////////////////////////////////////////////////////////////////////////////////////
// реализация cTileMap
//////////////////////////////////////////////////////////////////////////////////////////
void cTileMap::updateMap(const Vect2i& pos1,const Vect2i& pos2,UpdateMapType type)
{
	MTAuto enter(lock_update_rect);
	UpdateRect rc;
	rc.p1=pos1;
	rc.p2=pos2;
	rc.type=type;

	if(enable_debug_rect)
	{
		DebugRect rc;
		rc.p1=pos1;
		rc.p2=pos2;
		rc.time=debug_fade_interval;
		debug_rect.push_back(rc);
	}

	//point_offset На столько точка может выступать за границы тайла (реально на 16 может для самого грубого тайла)
	const int point_offset=8;
	Vect2i sz(tileSize_.x*tileNumber_.x, tileSize_.y*tileNumber_.y);
	rc.p1.x=max(rc.p1.x-point_offset,0);
	rc.p2.x=min(rc.p2.x+point_offset,sz.x-1);
	rc.p1.y=max(rc.p1.y-point_offset,0);
	rc.p2.y=min(rc.p2.y+point_offset,sz.y-1);

	update_rect.push_back(rc);
}

void cTileMap::updateTileZ(int x_tile,int y_tile)
{
	int zmin, zmax, zavr;
	vMap.getZMinMaxAvr(x_tile*tileSize().x,y_tile*tileSize().y,tileSize().x,tileSize().y, zmin, zmax, zavr);

	sTile& s=GetTile(x_tile,y_tile);
	s.zmin=zmin;
	s.zmax=zmax;
	s.zavr=zavr;
	zMax_ = max(zMax_, zmax);

	int x0 = x_tile*TILEMAP_SIZE;
	int x1 = (x_tile + 1)*TILEMAP_SIZE;
	int y0 = y_tile*TILEMAP_SIZE;
	int y1 = (y_tile + 1)*TILEMAP_SIZE;

	short* left = tileBordersX_ + (x0 << tileBordersShr_.x) + y0;
	short* top = tileBordersY_ + (y0 << tileBordersShr_.y) + x0;
	for(int i = 0; i < TILEMAP_SIZE; i++){
		*(left++) = vMap.getAltC(x0, y0 + i);
		*(top++) = vMap.getAltC(x0 + i, y0);
	}

	xassert(left - tileBordersX_ <= tileBordersSize_.x);
	xassert(top - tileBordersY_ <= tileBordersSize_.y);
}

Vect2f cTileMap::CalcZ(Camera* camera)
{
	float tx=tileSize().x * GetTileScale().x;
	float ty=tileSize().y * GetTileScale().y;

	Vect2f z(1e20f,1e-20f);

	for(int x=0;x<tileNumber_.x;x++)
	for(int y=0;y<tileNumber_.y;y++)
	{
		sTile& s=GetTile(x,y);
		Vect3f c0,c1;
		c0.x=x*tx;
		c0.y=y*ty;
		c0.z=s.zmin;
		c1.x=c0.x+tx;
		c1.y=c0.y+ty;
		c1.z=s.zmax;

		if(camera->TestVisible(c0,c1))//Не оптимально, лучше чераз обращение к pTestGrid
		{
			Vect3f p[8]=
			{
				Vect3f(c0.x,c0.y,c0.z),
				Vect3f(c1.x,c0.y,c0.z),
				Vect3f(c0.x,c1.y,c0.z),
				Vect3f(c1.x,c1.y,c0.z),
				Vect3f(c0.x,c0.y,c1.z),
				Vect3f(c1.x,c0.y,c1.z),
				Vect3f(c0.x,c1.y,c1.z),
				Vect3f(c1.x,c1.y,c1.z),
			};

			for(int i=0;i<8;i++)
			{
				Vect3f o=camera->GetMatrix()*p[i];
				if(o.z<z.x)
					z.x=o.z;
				if(o.z>z.y)
					z.y=o.z;
			}
		}
	}

	if(z.y<z.x){
		z=camera->GetZPlane();
	}
	else{
		float addz=(z.y-z.x)*1e-2;
		addz=max(addz,10.0f);
		z.x-=addz;
		z.y+=addz;
		if(z.x<camera->GetZPlane().x)
			z.x=camera->GetZPlane().x;
		if(z.y>camera->GetZPlane().y)
			z.y=camera->GetZPlane().y;
	}

	return z;
}

sBox6f cTileMap::CalcShadowReciverInSpace(Camera* camera,const Mat4f& matrix)
{
	sBox6f box;
	float tx=tileSize().x * GetTileScale().x;
	float ty=tileSize().y * GetTileScale().y;

	for(int x=0;x<tileNumber_.x;x++)
		for(int y=0;y<tileNumber_.y;y++){
			sTile& s=GetTile(x,y);
			Vect3f c0,c1;
			c0.x=x*tx;
			c0.y=y*ty;
			c0.z=s.zmin;
			c1.x=c0.x+tx;
			c1.y=c0.y+ty;
			c1.z=s.zmax;

			if(camera->TestVisible(c0,c1)){
				Vect3f p[8]= {
					Vect3f(c0.x,c0.y,c0.z),
					Vect3f(c1.x,c0.y,c0.z),
					Vect3f(c0.x,c1.y,c0.z),
					Vect3f(c1.x,c1.y,c0.z),
					Vect3f(c0.x,c0.y,c1.z),
					Vect3f(c1.x,c0.y,c1.z),
					Vect3f(c0.x,c1.y,c1.z),
					Vect3f(c1.x,c1.y,c1.z),
				};

				for(int i=0;i<8;i++){
					Vect3f px;
					matrix.xformCoord(p[i], px);
					box.addPoint(px);
				}
			}
		}

	return box;
}


void cTileMap::BuildRegionPoint()
{
	MTAuto enter(lock_update_rect);

	int counter = 0;
	int counterRegion = 0;

	vector<UpdateRect>::iterator it;
	FOR_EACH(update_rect,it){
		UpdateRect& r=*it;
		Vect2i& pos1=r.p1;
		Vect2i& pos2=r.p2;

		xassert(pos1.y<=pos2.y&&pos1.x<=pos2.x);
		int dx=tileSize().x,dy=tileSize().y;
		int j1=pos1.y/dy,j2=min(pos2.y/dy,tileNumber().y-1);
		int i1=pos1.x/dx,i2=min(pos2.x/dx,tileNumber().x-1);
		for(int j=j1;j<=j2;j++)
		for(int i=i1;i<=i2;i++)
		{
			sTile& p=GetTile(i,j);
			if((r.type&UPDATEMAP_HEIGHT) && p.update_zminmmax_time!=update_zminmmax_time)
			{
				p.update_zminmmax_time=update_zminmmax_time;
				updateTileZ(i,j);
				counter++;
			}
			if(r.type&(UPDATEMAP_HEIGHT|UPDATEMAP_REGION))
				p.setAttribute(ATTRTILE_UPDATE_VERTEX);
			if(r.type&UPDATEMAP_TEXTURE)
				p.setAttribute(ATTRTILE_UPDATE_TEXTURE);
		}
		counterRegion++;
	}

	update_rect.clear();

	statistics_add(TilesUpdated, counter);
	statistics_add(RegionsUpdated, counterRegion);

	update_zminmmax_time++;
}

Vect3f cTileMap::To3D(const Vect2f& pos)
{
	Vect3f p;
	p.x=pos.x;
	p.y=pos.y;

	int x = round(pos.x), y = round(pos.y);
	if(x >= 0 && x < vMap.H_SIZE && y >= 0 && y < vMap.V_SIZE)
	{
		p.z=vMap.getZf(x,y);
	}else
		p.z=0;
	return p;
}

void cTileMap::Animate(float dt)
{
	animationTime_ = fmodFast(animationTime_ + dt*0.001f, 1e+7f);
	if(enable_debug_rect)
	{
		for(list<DebugRect>::iterator it=debug_rect.begin();it!=debug_rect.end();)
		{
			DebugRect& r=*it;
			r.time-=dt;
			if(r.time<0)
				it=debug_rect.erase(it);
			else
				it++;
		}
	}
}

bool cTileMap::setMaterial(int material, eBlendMode MatMode)
{
	if(material < miniDetailTexturesNumber){
		Color4f color=GetZeroplastColor(material);
		if(MatMode!=ALPHA_BLEND)
			color.a=1;
		gb_RenderDevice3D->dtAdvance->SetTileColor(color);
		cTexture* texture = Option_DetailTexture ? miniDetailTexture(material).texture : 0;
		gb_RenderDevice3D->dtAdvance->SetMaterialTilemap(scene()->GetShadowIntensity(), texture, miniDetailTextureResolution());
		return false;
	}
	else{
		PlacementZoneMaterial& mat = placementZoneMaterials_[material - miniDetailTexturesNumber];
		if(mat.shaderType == LAVA){
			gb_RenderDevice3D->SetTexture(1, mat.texture);
			lavaShader_->setTextureScale(mat.textureScale, mat.volumeTextureScale);
			lavaShader_->SetColors(mat.lavaColor, mat.colorAmbient);
			lavaShader_->SetTime(animationTime_*mat.speed);
			lavaShader_->Select();
			return false;
		}
		else{
			iceShader_->beginDraw(mat.texture, mat.textureBump, 0, mat.textureCleft, 30, 0);
			return true;
		}
	}
}

void cTileMap::DrawLines()
{
	if(enable_debug_rect)
	{
		list<DebugRect>::iterator it;
		FOR_EACH(debug_rect,it)
		{
			DebugRect& r=*it;
			Color4c c(255,255,255,round(255*r.time/debug_fade_interval));
			Vect3f p0,p1,p2,p3;
			p0=To3D(Vect2f(r.p1.x,r.p1.y));
			p1=To3D(Vect2f(r.p2.x,r.p1.y));
			p2=To3D(Vect2f(r.p2.x,r.p2.y));
			p3=To3D(Vect2f(r.p1.x,r.p2.y));

			gb_RenderDevice->DrawLine(p0,p1,c);
			gb_RenderDevice->DrawLine(p1,p2,c);
			gb_RenderDevice->DrawLine(p2,p3,c);
			gb_RenderDevice->DrawLine(p3,p0,c);
		}
	}
}

////////////////////////////////////////////////////
int cTileMap::MiniDetailTexture::resolution = 4;

cTileMap::MiniDetailTexture::MiniDetailTexture()
{
	textureName = "Scripts\\Resource\\balmer\\noise.tga";
}

void cTileMap::MiniDetailTexture::setTexture()
{
	if(!textureName.empty())
		texture = GetTexLibrary()->GetElementMiniDetail(textureName.c_str(), resolution);
	
}

void cTileMap::MiniDetailTexture::serialize(Archive& ar)
{
	ar.serialize(ResourceSelector(textureName, textureOptions), "textureName", 0); // CONVERSION
	if(ar.isInput() && !ar.isEdit())
		ar.serialize(textureName, "textureNameOriginal", 0); // CONVERSION

	if(ar.isInput())
		setTexture();
}


////////////////////////////////////////////////////

cTileMap::PlacementZoneMaterial::PlacementZoneMaterial()
{
	shaderType = ICE;

	minimapColor.set(255, 255, 255, 0);

	lavaColor.set(0.8f, 0.8f, 0.4f, 1.f);
	colorAmbient.set(0.5f, 0, 0, 1.f);
	speed = 0.01f;
	textureScale = 0.02f;
	volumeTextureScale = 0.03f;
//	textureName = "Scripts\\Resource\\balmer\\lava.tga";

	textureName = "Scripts\\Resource\\balmer\\snow.tga";
	textureBumpName = "Scripts\\Resource\\balmer\\snow_bump.tga";
	textureCleftName = "Scripts\\Resource\\balmer\\ice_cleft.tga";
}

void cTileMap::PlacementZoneMaterial::serialize(Archive& ar)
{
	ar.serialize(shaderType, "shaderType", "Тип шейдера");
	if(!ar.serialize(ResourceSelector(textureName, textureOptions), "textureName", "Имя текстуры")){ // CONVERSION
		ar.openStruct(*this, "miniDetailTexture", 0);
		ar.serialize(textureName, "textureName", 0);
		ar.closeStruct("miniDetailTexture");
	}
	ar.serialize(minimapColor, "minimapColor", "Цвет зоны на миникарте");
	if(shaderType == LAVA){
		ar.serialize(lavaColor, "color", "Цвет");
		ar.serialize(colorAmbient, "colorAmbient", "Цвет амбиент");
		ar.serialize(textureScale, "textureScale", "Масштаб текстуры");
		ar.serialize(volumeTextureScale, "volumeTextureScale", "Масштаб объемной текстуры");
	}
	else{
	    ar.serialize(ResourceSelector(textureBumpName, textureOptions), "textureBump", "Bump текстура");
	    ar.serialize(ResourceSelector(textureCleftName, textureOptions), "textureCleft", "Текстура трещин");
		if(ar.isInput()){
			textureBump = GetTexLibrary()->GetElement3D(textureBumpName.c_str(), "Bump");
			textureCleft = GetTexLibrary()->GetElement3D(textureCleftName.c_str());
		}
	}

	if(ar.isInput() && !textureName.empty())
		texture = GetTexLibrary()->GetElement3D(textureName.c_str());
}

float cTileMap::getZ(int x,int y) const
{
	x = clamp(x, 0, vMap.H_SIZE - 1);
	y = clamp(y, 0, vMap.V_SIZE - 1);

	if(!(x & TILEMAP_SIZE - 1)){
		xassert((x << tileBordersShr_.x) + y < tileBordersSize_.y);
		return tileBordersX_[(x << tileBordersShr_.x) + y]*heightFractionInv_;
	}

	if(!(y & TILEMAP_SIZE - 1)){
		xassert((y << tileBordersShr_.y) + x < tileBordersSize_.y);
		return tileBordersY_[(y << tileBordersShr_.y) + x]*heightFractionInv_;
	}

	return vMap.getZf(x, y);

	//if(cChaos::CurrentChaos() && zi<1)
	//	zi=-15;
}

void cTileMap::getNormal(int x, int y, int step, Vect3f& normal) const
{
	start_timer_auto();
	step=step>>1;
	float dzx = getZ(x + step, y) - getZ(x - step, y);
	float dzy = getZ(x, y + step) - getZ(x, y - step);
	normal.set(-dzx,-dzy,step*2);
	normal.normalize();
}
