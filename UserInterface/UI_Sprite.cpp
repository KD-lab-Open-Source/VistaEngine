#include "StdAfx.h"
#include "UI_Sprite.h"
#include "Serialization\StringTableImpl.h"
#include "Serialization\LibraryWrapper.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\SerializationFactory.h"
#include "FileUtils\FileUtils.h"
#include "Render\src\Texture.h"
#include "UI_RenderBase.h"
#include "Util\DebugPrm.h"

extern float cycle(float f, float size);
extern bool isUnderEditor();
const char* getLocDataPath();

DECLARE_SEGMENT(UI_Sprite)
REGISTER_CLASS(UI_Texture, UI_Texture, "текстура");
WRAP_LIBRARY(UI_TextureLibrary, "UI_TextureLibrary", "UI_TextureLibrary", "Scripts\\Content\\UI_TextureLibrary", 0, 0);

// ------------------- UI_Texture

UI_Texture::UI_Texture(const char* fileName /*= ""*/, bool hasResolutionVersion /*= true*/, bool hasRaceVersion /*= false*/) : texture_(0)
{
	localized_ = false;
	setFileName(fileName);
	hasResolutionVersion_ = hasResolutionVersion;
	hasRaceVersion_ = hasRaceVersion;

	textureSize_ = Vect2i(0,0);
	textureSizeCurrent_ = Vect2i(0,0);

	textureSizeInit_ = false;
}

UI_Texture::UI_Texture(const UI_Texture& src)
: texture_(0)
{
	fileName_ = src.fileName_;
	localized_ = src.localized_;
	hasResolutionVersion_ = src.hasResolutionVersion_;
	hasRaceVersion_ = src.hasRaceVersion_;

	textureSize_ = src.textureSize_;
	textureSizeCurrent_ = src.textureSizeCurrent_;

	textureSizeInit_ = src.textureSizeInit_;
}

void UI_Texture::buildLocPath(const char* in, string& out)
{
	out = getLocDataPath();
	out += "UI_Textures\\";
	out += extractFileName(in);
}

void UI_Texture::updateFileNameProxy()
{
	if(fileName_.empty())
		fileNameProxy_.clear();
	else if(localized_)
		buildLocPath(fileName_.c_str(), fileNameProxy_);
	else
		fileNameProxy_ = fileName_;
}

// если файл лежит в папке с локкитом, то
// путь нужно обрезать до имени и выставить галку, что текстура локализованная
void UI_Texture::setFileName(const char* name)
{
	xassert(name);
	string locPath;
	buildLocPath(name, locPath);
	if(localized_ = isFileExists(locPath.c_str()))
		fileName_ = extractFileName(name);
	else
		fileName_ = name;
	updateFileNameProxy();
}

const char* UI_Texture::textureFileName() const
{
	return fileNameProxy_.c_str();
}

void UI_Texture::serialize(Archive& ar)
{
	ar.serialize(localized_, "localized", 0);
	ar.serialize(fileName_, "fileName_", "&имя файла");
	if(ar.isInput())
		updateFileNameProxy();
	else 
		ExportInterface::export(fileNameProxy_.c_str());

	ar.serialize(hasResolutionVersion_, "hasResolutionVersion_", "поддержка разных разрешений");
	ar.serialize(hasRaceVersion_, "hasRaceVersion_", "поддержка разных рас");

	if(isUnderEditor()){
		if(ar.isOutput() && !ar.isEdit() && textureSizeInit_){
			if(textureSizeCurrent_ != Vect2i::ZERO)
				textureSize_ = textureSizeCurrent_;
		}
		else {
			if(ar.isInput())
				initSize();
		}
	}

	ar.serialize(textureSize_, "textureSize", 0);
}

bool UI_Texture::createTexture() const
{
	releaseTexture();

	if(fileName_.empty())
		return false;

	if(!showDebugInterface.disableTextures)
		texture_ = UI_RenderBase::instance().createTexture(textureFileName());

	return true;
}

void UI_Texture::releaseTexture() const
{
	UI_RenderBase::instance().releaseTexture(texture_);
}

bool UI_Texture::needRemapCoords() const
{
	if(!textureSizeInit_ || textureSize_ == textureSizeCurrent_ ||textureSize_ == Vect2i::ZERO || textureSizeCurrent_ == Vect2i::ZERO)
		return false;

	float x_ratio = float(textureSizeCurrent_.x) / float(textureSize_.x);
	float y_ratio = float(textureSizeCurrent_.y) / float(textureSize_.y);

	return fabs(x_ratio - y_ratio) > FLT_EPS;
}

Rectf UI_Texture::remapCoords(const Rectf& coords) const
{
	if(!textureSizeInit_ || textureSize_ == textureSizeCurrent_ ||textureSize_ == Vect2i::ZERO || textureSizeCurrent_ == Vect2i::ZERO)
		return coords;

	float x_ratio = float(textureSize_.x) / float(textureSizeCurrent_.x);
	float y_ratio = float(textureSize_.y) / float(textureSizeCurrent_.y);

	return coords * Vect2f(x_ratio, y_ratio);
}

cTexture* UI_Texture::texture(bool noCreate) const
{
	if(!texture_){
		if(noCreate)
			return 0;
		createTexture();
	}

	return texture_;
}

void UI_Texture::initSize()
{
	if(textureSizeInit_)
		return;

	textureSizeInit_ = true;

	bool need_release = false;
	if(!texture_){
		createTexture();
		need_release = true;
	}

	if(texture_)
		textureSizeCurrent_ = Vect2i(texture_->GetWidth(), texture_->GetHeight());

	if(need_release)
		releaseTexture();
}

// ------------------- UI_Sprite

const  UI_Sprite UI_Sprite::ZERO;

UI_Sprite::UI_Sprite()
: textureCoords_(0,0,1,1)
, diffuseColor_(255, 255, 255, 255)
, saturation_(1.0f)
{
	runtimeTexture_ = 0;
	coordsRemapped_ = false;
#ifndef _FINAL_VERSION
	addRefCount_ = 0;
#endif
}

UI_Sprite::UI_Sprite(const char* filename)
: textureCoords_(0, 0, 1, 1)
, diffuseColor_(255, 255, 255, 255)
, saturation_(1.0f)
{
	coordsRemapped_ = false;
	runtimeTexture_ = UI_RenderBase::instance().createTexture(filename);
#ifndef _FINAL_VERSION
	addRefCount_ = 0;
#endif
}

UI_Sprite::~UI_Sprite()
{
}

void UI_Sprite::addRef()
{
	if(cTexture* tex = texture()){
#ifndef _FINAL_VERSION
		addRefCount_++;
#endif
		tex->AddRef();
	}
}

void UI_Sprite::decRef()
{
	if(cTexture* tex = texture(true)){
#ifndef _FINAL_VERSION
		xassert(addRefCount_ && "Некорректный вызов UI_Sprite::decRef()");
		addRefCount_--;
#endif
		tex->DecRef();
	}
}

void UI_Sprite::release()
{
	if(texture_)
		texture_->releaseTexture();
	else
		UI_RenderBase::instance().releaseTexture(runtimeTexture_);
}

void UI_Sprite::serialize(Archive& ar)
{
	ar.serialize(texture_, "texture_", "&текстура");
	if(texture_.key() >= 0){
		ar.serialize(textureCoords_, "textureCoords_", "текстурные координаты");
		ar.serialize(diffuseColor_, "diffuseColor", "цвет");
		ar.serialize(saturation_, "saturation", "насыщенность");
	}

	if(isUnderEditor()){
		if(ar.isInput() && !coordsRemapped_ && texture_){
			coordsRemapped_ = true;
			if(texture_->needRemapCoords())
				textureCoords_ = texture_->remapCoords(textureCoords_);
		}
	}
}

void UI_Sprite::setTextureReference(const UI_TextureReference& ref)
{
	texture_ = ref;
}

bool UI_Sprite::isAnimated() const
{
	return isEmpty() ? false : texture()->frameNumber() > 1;
}

cTexture* UI_Sprite::texture(bool noCreate) const
{
	if(runtimeTexture_)
		return runtimeTexture_;

	if(const UI_Texture* texture = &*texture_)
		if(texture_.key() >= 0)
			return texture->texture(noCreate);
	return 0;
}

Vect2f UI_Sprite::size() const
{
	return texture() ?
		Vect2f(float(texture()->GetWidth()) * textureCoords().width(),  float(texture()->GetHeight()) * textureCoords().height()) :
	Vect2f::ZERO;
}

float UI_Sprite::phase(float time, bool cycled) const
{
	return isAnimated() ?
		((time*=1000.f) > texture()->GetTotalTime() ? 
		(cycled ? cycle(time, (float)texture()->GetTotalTime()) / texture()->GetTotalTime() : 1.f) :
	time / texture()->GetTotalTime()) :
	0.f;
}

// ------------------- UI_LibSprite

void UI_LibSprite::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);
	ar.serialize(static_cast<UI_Sprite&>(*this), "sprite", "Спрайт");
}

// ------------------- UI_UnitSprite

UI_UnitSprite::UI_UnitSprite() : UI_Sprite(),
spriteOffset_(Vect2f::ZERO)
{
	useLegionColor_ = false;
	spriteScale_ = 1.f;
	hideScale_ = 0.2f;
	minPerspectiveScale_ = 0.4f;
	maxPerspectiveScale_ = 2.0f;
}

UI_UnitSprite::~UI_UnitSprite()
{
}

void UI_UnitSprite::serialize(Archive& ar)
{
	ar.serialize(static_cast<UI_Sprite&>(*this), "sprite", "Спрайт");
	ar.serialize(useLegionColor_, "useLegionColor", "Красить в цвет легиона");
	ar.serialize(spriteOffset_, "spriteOffset", "Смещение значка");
	ar.serialize(spriteScale_, "spriteScale", "Масштаб значка");
	if(ar.openBlock("perspective", "перспектива")){
		ar.serialize(RangedWrapperf(minPerspectiveScale_, 0.f, 1.f), "minPerspectiveScale", "Минимальный маштаб уменьшения при отдалении");
		ar.serialize(RangedWrapperf(maxPerspectiveScale_, minPerspectiveScale_, 10.f), "maxPerspectiveScale", "Максимальный маштаб увеличения при приближении");
		ar.serialize(RangedWrapperf(hideScale_, 0.f, minPerspectiveScale_), "hideScale", "Маштаб исчезновения при отдалении");
		ar.closeBlock();
	}	
}

Vect2f UI_UnitSprite::size() const
{
	Vect2f sz(UI_Sprite::size());
	sz *= spriteScale_;
	return sz;
}