#include <stdafxRD.h>
#include "LensFlare.h"
#include "OcclusionQuery.h"
#include "cCamera.h"
#include "D3DRender.h"
#include "Render\3dx\Node3dx.h"
#include "VisGeneric.h"
#include "Serialization\Serialization.h"
#include "Serialization\ResourceSelector.h"

//////////////////////////////////////////////////////////////////////////////
LensFlareSprite::~LensFlareSprite()
{
	RELEASE(texture_);
}

//////////////////////////////////////////////////////////////////////////////
void LensFlareSprite::init()
{
    if(texture_ && stricmp(filename_.c_str(), texture_->name()) == 0)
        return;

    if(texture_){
        texture_->Release();
        texture_ = 0;
    }
    if(!filename_.empty()){
        texture_ = gb_VisGeneric->CreateTexture(filename_.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////////
void LensFlareSprite::serialize(Archive& ar)
{
    ar.serialize(position_, "position", "&Положение");
    ar.serialize(radius_, "radius", "&Радиус");
    ar.serialize(color_, "color", "&Цвет");
    ar.serialize(additiveBlending_, "additiveBlending", "Адитивный блендинг");
	static ResourceSelector::Options options("*.tga", "Resource\\FX\\Textures", "Please, select texture", true);
	ar.serialize(ResourceSelector(filename_, options), "filename", "Текстура");
    //if(ar.isInput() && ar.isEdit())
    //    init();
}

//////////////////////////////////////////////////////////////////////////////
cTexture* LensFlareSprite::texture() const
{
    return texture_;
}

//////////////////////////////////////////////////////////////////////////////
LensFlare::LensFlare()
{
	reserve(10);
	push_back(LensFlareSprite(-0.1f, 0.3f,  "Scripts\\Resource\\Textures\\flare01.tga",				Color4f(1.0f, 0.9f,  0.6f,  0.2f)));
	push_back(LensFlareSprite(0.0f,  0.5f,  "Scripts\\Resource\\Textures\\flare01.tga",				Color4f(1.0f, 1.0f,  1.0f,  0.5f)));
	push_back(LensFlareSprite(0.0,   0.1f,  "Scripts\\Resource\\Textures\\flare01.tga",				Color4f(1.0f, 1.0f,  1.0f,  0.5f)));
	push_back(LensFlareSprite(0.15f, 0.13f, "Scripts\\Resource\\Textures\\flare_ring01.tga",			Color4f(1.0f, 0.7f,  0.7f,  0.05f)));
	push_back(LensFlareSprite(0.45f, 0.05f, "Scripts\\Resource\\Textures\\flare_radial_linear.tga",	Color4f(1.0f, 0.75f, 0.7f,  0.4f)));
	push_back(LensFlareSprite(0.37f, 0.1f,  "Scripts\\Resource\\Textures\\flare_radial_linear.tga",	Color4f(0.9f, 1.0f,  0.8f,  0.05f)));
	push_back(LensFlareSprite(0.73f, 0.4f,  "Scripts\\Resource\\Textures\\flare_radial_linear.tga",  Color4f(0.8f, 0.9f,  1.0f,  0.2f)));
	push_back(LensFlareSprite(0.75f, 0.05f, "Scripts\\Resource\\Textures\\flare_ring03.tga",       	Color4f(1.0f, 0.9f,  0.8f,  0.1f)));
	push_back(LensFlareSprite(0.85f, 0.2f,  "Scripts\\Resource\\Textures\\flare_ring02.tga",       	Color4f(1.0f, 0.75f, 0.25f, 0.12f)));
	push_back(LensFlareSprite(1.0f,  0.5f,  "Scripts\\Resource\\Textures\\flare_radial_linear.tga",  Color4f(0.87f,0.95f, 1.0f,  0.1f)));
}

//////////////////////////////////////////////////////////////////////////////
bool LensFlare::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(static_cast<std::vector<LensFlareSprite>&>(*this), name, nameAlt);
}

//////////////////////////////////////////////////////////////////////////////

LensFlareRenderer::LensFlareRenderer()
: BaseGraphObject(SCENENODE_OBJECT)
, occlusionQuery_(*new cOcclusionQuery)
, sourceRadius_(0.0f)
, sourcePosition_(Vect3f::ZERO)
, isVisible_(true)
, isEnabled_(false)
, showGlowSprite_(false)
, position_(MatXf::ID)

, cameraClipMin_(100.0f)
, cameraClipMax_(13000.0f)
, opacity_ (0)
{
	occlusionQuery_.Init();
}

//////////////////////////////////////////////////////////////////////////////

void LensFlareRenderer::drawFlare2D(const Vect2f& screenPoint, const LensFlare& flare, float alpha)
{
	cScene* scene = scene_;
	cD3DRender* renderDevice = static_cast<cD3DRender*>(gb_RenderDevice);
	////

	Vect2f screenCenter(renderDevice->GetSizeX() * 0.5f, renderDevice->GetSizeY() * 0.5f);
	float halfSizeMultiplier = 0.5f * float(renderDevice->GetSizeX());
	Vect2f flareAxis = (screenCenter - screenPoint) * 2.0f;

    renderDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    renderDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

	LensFlare::const_iterator it;
	FOR_EACH(flare, it){
		const LensFlareSprite& sprite = *it;
		Color4c color = sprite.color();
		color.a = unsigned char(color.a * alpha);
		if(color.a<2)
			continue;

		float halfSize = sprite.radius() * halfSizeMultiplier;
		Vect2f spritePoint = screenPoint + flareAxis * sprite.position();

		float x1 = spritePoint.x - halfSize;
		float y1 = spritePoint.y - halfSize;
		float x2 = spritePoint.x + halfSize;
		float y2 = spritePoint.y + halfSize;

		renderDevice->SetNoMaterial(sprite.additiveBlending() ? ALPHA_ADDBLENDALPHA : ALPHA_BLEND, MatXf::ID, 0, sprite.texture());
		renderDevice->SetVertexShader(0);
		renderDevice->SetPixelShader(0);

		cVertexBuffer<sVertexXYZWDT1>* vertexBuffer = renderDevice->GetBufferXYZWDT1();
		sVertexXYZWDT1* vertices = vertexBuffer->Lock(6);
		vertices[0].z = vertices[1].z = vertices[2].z = vertices[4].z = 0.001f;
		vertices[0].w = vertices[1].w = vertices[2].w = vertices[4].w = 0.001f;
		vertices[0].diffuse = vertices[1].diffuse = vertices[2].diffuse = vertices[4].diffuse = color;

		vertices[0].x = x1;
		vertices[0].y = y1;
		vertices[0].uv[0] = 0.0f;
		vertices[0].uv[1] = 0.0f;

		vertices[1].x = x2;
		vertices[1].y = y1;
		vertices[1].uv[0] = 1.0f;
		vertices[1].uv[1] = 0.0f;

		vertices[2].x = x2;
		vertices[2].y = y2;
		vertices[2].uv[0] = 1.0f;
		vertices[2].uv[1] = 1.0f;

		vertices[4].x = x1;
		vertices[4].y = y2;
		vertices[4].uv[0] = 0.0f;
		vertices[4].uv[1] = 1.0f;

		vertices[3] = vertices[2];
		vertices[5] = vertices[0];
		vertexBuffer->Unlock(6);
		vertexBuffer->DrawPrimitive(PT_TRIANGLELIST, 2);
	}
    renderDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    renderDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
}


//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::drawSprite(Camera* camera, const Vect3f& point, const LensFlareSprite& sprite, float alpha)
{
    cScene* scene = scene_;
	cD3DRender* renderDevice = static_cast<cD3DRender*>(gb_RenderDevice);
	////

	Vect3f vect = camera->GetPos() - sourcePosition_;
	float distance = vect.norm();
	vect.normalize();
	Vect3f xAxis;
	Vect3f yAxis;
	xAxis.cross(vect, Vect3f::I);
    yAxis.cross(vect, xAxis);    

	float radius = sprite.radius() * distance;

	xAxis.normalize(radius * 2.0f);
	yAxis.normalize(radius * 2.0f);

	renderDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    renderDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	renderDevice->SetNoMaterial(sprite.additiveBlending() ? ALPHA_ADDBLENDALPHA : ALPHA_BLEND, MatXf::ID, 0, sprite.texture());

	cVertexBuffer<sVertexXYZDT1>* vertexBuffer = renderDevice->GetBufferXYZDT1();
	sVertexXYZDT1* vertices = vertexBuffer->Lock(6);

	Color4c color = sprite.color(); color.a *= alpha;
	vertices[0].diffuse = vertices[1].diffuse = vertices[2].diffuse = vertices[4].diffuse = color;

	Vect3f origin = point - (xAxis + yAxis) * 0.5f;

	vertices[0].pos = origin;
	vertices[0].uv[0] = 0.0f;
	vertices[0].uv[1] = 0.0f;

	vertices[1].pos = origin + xAxis;
	vertices[1].uv[0] = 1.0f;
	vertices[1].uv[1] = 0.0f;

	vertices[2].pos = origin + xAxis + yAxis;
	vertices[2].uv[0] = 1.0f;
	vertices[2].uv[1] = 1.0f;

	vertices[4].pos = origin + yAxis;
	vertices[4].uv[0] = 0.0f;
	vertices[4].uv[1] = 1.0f;

	vertices[3] = vertices[2];
	vertices[5] = vertices[0];
	vertexBuffer->Unlock(6);
	vertexBuffer->DrawPrimitive(PT_TRIANGLELIST, 2);
}

//////////////////////////////////////////////////////////////////////////////
inline bool objectBoxTest(const Vect3f& pos, Plane* box)
{
	bool res = false;

	float leftDist = box[1].distance(pos);
	float rightDist = box[2].distance(pos);
	float topDist = box[3].distance(pos);
	float bottomDist = box[4].distance(pos);

	return
			(leftDist >= 0 && rightDist >= 0)
		&&	(topDist >= 0 && bottomDist >= 0);
};

//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::Draw(Camera* camera)
{
	if(!isVisible_ || !isEnabled_)
		return;

	cD3DRender* renderDevice = static_cast<cD3DRender*>(gb_RenderDevice);
	
	DWORD oldFogState = renderDevice->GetRenderState(D3DRS_FOGENABLE);
	DWORD oldAlphaBlend=renderDevice->GetRenderState(D3DRS_ALPHABLENDENABLE);
	renderDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
	const Mat4f& matViewProjScr = camera->matViewProjScr;

	Vect3f point;
	matViewProjScr.xformCoord(sourcePosition_, point);

	const int sampleSize = 5;
	const int numPoints = sampleSize * sampleSize;

	if(occlusionQuery_.IsInit()){
		Vect3f vect = sourcePosition_ - camera->GetPos();
		float distance = vect.norm();
		vect.normalize();
		//vect *= sourceRadius_ * 1.5f;
		Vect3f sampleCenter = camera->GetPos() + vect * min(cameraClipMax_ * 0.9f, distance);
		
		if(camera->GetCameraPass() == SCENENODE_OBJECTSORT){
			if(showGlowSprite_)
				drawSprite(camera, sampleCenter, glowSprite_, min(1.0f, opacity_ * 2.0f));
		} 
		else{
			vect.normalize();
			Vect3f xAxis;
			Vect3f yAxis;
			xAxis.cross(vect, Vect3f::I);
			yAxis.cross(vect, xAxis);        

			points_.reserve(numPoints);
			points_.clear();

			xAxis.normalize();
			yAxis.normalize();
			for(int i = 0; i < numPoints; ++i){
				float angle = M_PI * 2.0f * float(i) / float(numPoints);
				Vect3f testPoint = sampleCenter + (xAxis * cos(angle) + yAxis * sin(angle)) * sourceRadius_ * 0.5f;
				points_.push_back(testPoint);
			}


			drawFlare2D(Vect2f(point), lensFlare_, opacity_);
			opacity_ = clamp(float(occlusionQuery_.VisibleCount()) / float(numPoints), 0.0f, 1.0f);
			if(!points_.empty())
				occlusionQuery_.Test(&points_[0], points_.size());
		}
	}
	renderDevice->SetRenderState(D3DRS_FOGENABLE, oldFogState);
	renderDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
}

//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::PreDraw(Camera* camera)
{
	camera->Attach(SCENENODE_OBJECTSPECIAL, this);
	camera->Attach(SCENENODE_OBJECTSORT, this);
}
//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::init()
{
	LensFlare::iterator i;
	FOR_EACH(lensFlare_, i)
		i->init();
	if(showGlowSprite_)
		glowSprite_.init();
}
//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::serialize(Archive& ar)
{
	ar.serialize(isEnabled_, "isEnabled", "Включить");
	ar.serialize(showGlowSprite_, "showGlowSprite", "Показывать ореол");
	ar.serialize(glowSprite_, "glowSprite", showGlowSprite_ ? "Ореол" : 0);
	ar.serialize(lensFlare_, "sprites", "Спрайты");
	if(ar.isInput() && isEnabled_)
		init();
}


//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::setFlareSource(const Vect3f& position, float radius)
{
	//position_.trans() = sourcePosition_ = flareSource->GetPosition().trans();
	position_.trans() = position;
	sourcePosition_ = position;
    sourceRadius_ = radius;
}

//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::setVisible(bool show)
{
	isVisible_ = show;
}

//////////////////////////////////////////////////////////////////////////////
LensFlareRenderer::~LensFlareRenderer()
{
	occlusionQuery_.Done();
	delete &occlusionQuery_;
}
