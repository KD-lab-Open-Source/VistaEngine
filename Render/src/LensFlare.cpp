#include <StdAfxRD.h>
#include "LensFlare.h"
#include "OcclusionQuery.h"
#include "cCamera.h"
#include "D3DRender.h"
#include "Render/SDLRenderDevice.h"
#include "Render/SDLWorldQuadRenderer.h"
#include "Render/3dx/Node3DX.h"
#include "VisGeneric.h"
#include "Serialization/Serialization.h"
#include "Serialization/ResourceSelector.h"

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
	// The D3D path drew these through the shared dynamic 2D buffer (GetBufferXYZWDT1),
	// pre-transformed, Z off. DrawSprite records them into the UI batch instead: it draws
	// over the scene in call order -- before the interface, which records later -- the
	// order the D3D pass sequence had. (That batch replays after the post-effect
	// composite, so monochrome does not grey the flare; see Render/PORTING.md.)
	Vect2f screenCenter(gb_RenderDevice->GetSizeX() * 0.5f, gb_RenderDevice->GetSizeY() * 0.5f);
	float halfSizeMultiplier = 0.5f * float(gb_RenderDevice->GetSizeX());
	Vect2f flareAxis = (screenCenter - screenPoint) * 2.0f;

	LensFlare::const_iterator it;
	FOR_EACH(flare, it){
		const LensFlareSprite& sprite = *it;
		Color4c color = sprite.color();
		color.a = (unsigned char)(color.a * alpha);
		if(color.a<2)
			continue;

		float halfSize = sprite.radius() * halfSizeMultiplier;
		Vect2f spritePoint = screenPoint + flareAxis * sprite.position();

		gb_RenderDevice->DrawSprite(int(spritePoint.x - halfSize), int(spritePoint.y - halfSize),
		                            int(halfSize * 2.0f), int(halfSize * 2.0f),
		                            0.0f, 0.0f, 1.0f, 1.0f, sprite.texture(), color, 0.0f,
		                            sprite.additiveBlending() ? ALPHA_ADDBLENDALPHA : ALPHA_BLEND);
	}
}


//////////////////////////////////////////////////////////////////////////////
void LensFlareRenderer::drawSprite(Camera* camera, const Vect3f& point, const LensFlareSprite& sprite, float alpha)
{
	// The glow: a camera-facing billboard at the sun, in world space. The D3D path drew it
	// through the shared dynamic buffer (GetBufferXYZDT1) with SetNoMaterial; the world-quad
	// renderer is that route's stand-in, depth test on as D3DRS_ZENABLE TRUE had it.
	SDLWorldQuadRenderer* quads = sdlWorldQuadRenderer();
	cSDLRenderDevice* dev = sdlRenderDevice();
	if(!quads || !dev)
		return;

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

	Color4c color = sprite.color(); color.a *= alpha;
	Vect3f origin = point - (xAxis + yAxis) * 0.5f;

	quads->SetCamera(camera);
	quads->SetMaterial(sprite.additiveBlending() ? ALPHA_ADDBLENDALPHA : ALPHA_BLEND, sprite.texture());
	quads->BeginDraw();
	sVertexXYZDT1* v = quads->Get();

	// Get's corners are (0,0), (0,1), (1,0), (1,1) -- the renderer's index pattern pairs
	// them into the same quad the D3D triangle list covered.
	v[0].pos = origin;
	v[0].diffuse = color;
	v[0].GetTexel().x = 0.0f; v[0].GetTexel().y = 0.0f;

	v[1].pos = origin + yAxis;
	v[1].diffuse = color;
	v[1].GetTexel().x = 0.0f; v[1].GetTexel().y = 1.0f;

	v[2].pos = origin + xAxis;
	v[2].diffuse = color;
	v[2].GetTexel().x = 1.0f; v[2].GetTexel().y = 0.0f;

	v[3].pos = origin + xAxis + yAxis;
	v[3].diffuse = color;
	v[3].GetTexel().x = 1.0f; v[3].GetTexel().y = 1.0f;

	quads->EndDraw();
	dev->drawWorldQuads();
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

	// Only the main view. PreDraw attached us to every camera, but the flare's 2D sprites
	// go through the UI batch, which replays once, on the swapchain -- the reflection,
	// shadow and lightmap cameras must not record them again.
	if(camera->getAttribute(ATTRCAMERA_REFLECTION | ATTRCAMERA_SHADOW | ATTRCAMERA_SHADOWMAP | ATTRCAMERA_MIRAGE))
		return;

	Vect3f point;
	camera->matViewProjScr.xformCoord(sourcePosition_, point);

	if(camera->GetCameraPass() == SCENENODE_OBJECTSORT){
		// The glow billboard, over the sorted transparents, where the D3D pass drew it.
		if(showGlowSprite_ && opacity_ > 0.0f){
			Vect3f vect = sourcePosition_ - camera->GetPos();
			float distance = vect.norm();
			vect.normalize();
			Vect3f sampleCenter = camera->GetPos() + vect * min(cameraClipMax_ * 0.9f, distance);
			drawSprite(camera, sampleCenter, glowSprite_, min(1.0f, opacity_ * 2.0f));
		}
		return;
	}

	// The D3D path measured the sun's visibility with cOcclusionQuery: 25 points on a
	// ring around it, z-tested by the GPU against the frame, opacity = the fraction that
	// passed. SDL GPU has no occlusion queries (Render/PORTING.md #20), so this is a CPU
	// stand-in: full opacity while the sun projects inside the viewport in front of the
	// camera, fading over an edge margin comparable to the ring as it leaves. What it
	// gives up is occlusion by terrain and objects -- with P2's camera the sun is high in
	// the sky whenever it is on screen at all.
	float opacity = 0.0f;
	Vect3f viewPos;
	camera->matView.xformCoord(sourcePosition_, viewPos);
	if(viewPos.z > 0.0f){
		const sViewPort& vp = camera->vp;
		const float margin = 0.05f * float(vp.Width);
		float fx = min(point.x - float(vp.X), float(vp.X + vp.Width) - point.x) / margin;
		float fy = min(point.y - float(vp.Y), float(vp.Y + vp.Height) - point.y) / margin;
		opacity = clamp(min(fx, fy), 0.0f, 1.0f);
	}
	opacity_ = opacity;

	if(opacity_ > 0.0f)
		drawFlare2D(Vect2f(point), lensFlare_, opacity_);
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
