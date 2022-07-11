#ifndef __UI_SPRITE_H__
#define __UI_SPRITE_H__
#include "XTL\Rect.h"
#include "xtl\Handle.h"
#include "XMath\Colors.h"
#include "Serialization\StringTableReferencePolymorphic.h"
#include "Serialization\StringTableReference.h"
#include "Serialization\StringTableBase.h"

class cTexture;
/// текстура
class UI_Texture : public PolymorphicBase
{
public:
	UI_Texture(const char* fileName = "", bool hasResolutionVersion = true, bool hasRaceVersion = false);

	UI_Texture(const UI_Texture& src);

	void serialize(Archive& ar);

	bool isEmpty() const { return fileName_.empty(); }

	const char* textureFileName() const;

	bool createTexture() const;
	void releaseTexture() const;

	/// текстура локализована
	bool localized() const { return localized_; }
	/// менялись ли пропорции текстуры
	bool needRemapCoords() const;
	/// пересчёт координат после изменения пропорций текстуры
	Rectf remapCoords(const Rectf& coords) const;
	void initSize();

	cTexture* texture(bool noCreate = false) const;

private:
	void buildLocPath(const char* in, string& out);
	void updateFileNameProxy();
	void setFileName(const char* name);

	bool localized_;

	bool hasResolutionVersion_;
	bool hasRaceVersion_;

	bool textureSizeInit_;
	/// запомненный размер текстуры, сериализуется
	Vect2i textureSize_;
	/// вычисленный при загрузке размер текстуры
	Vect2i textureSizeCurrent_;

	mutable cTexture* texture_;
	string fileName_;
	string fileNameProxy_;
};

typedef StringTable<StringTableBasePolymorphic<UI_Texture> > UI_TextureLibrary;
typedef StringTableReferencePolymorphic<UI_Texture, false> UI_TextureReference;

/// спрайт
class UI_Sprite
{
public:
	UI_Sprite();
	explicit UI_Sprite(const char* filename);
	~UI_Sprite();

	void serialize(Archive& ar);

	bool operator == (const UI_Sprite& obj) const { return texture_ == obj.texture_ && textureCoords_ == obj.textureCoords_; }

	bool isEmpty() const { return !texture(); }
	bool isAnimated() const;
	// передается текущее время в секундах с начала проигрывания - вычисляется фаза
	float phase(float time, bool cycled) const;

	cTexture* texture(bool noCreate = false) const;

	const Rectf& textureCoords() const { return textureCoords_; }

	Vect2f size() const;

	void setTextureReference(const UI_TextureReference& ref);
	void setTextureCoords(const Rectf& coords) { textureCoords_ = coords; }
	const UI_TextureReference& textureReference() const { return texture_; }

	void setSaturation(float saturation) { saturation_ = saturation; }
	float saturation() const{ return saturation_; }
	Color4c diffuseColor() const{ return diffuseColor_; }
	void setDiffuseColor(Color4c color){ diffuseColor_ = color; }

	static const UI_Sprite ZERO;

	void addRef();
	void decRef();

	void release();

private:

	UI_TextureReference texture_;
	Color4c diffuseColor_;
	float saturation_;
	Rectf textureCoords_;
	mutable cTexture* runtimeTexture_;

	bool coordsRemapped_;

#ifndef _FINAL_VERSION
	int addRefCount_;
#endif
};

class UI_LibSprite : public UI_Sprite, public StringTableBase {
public:
	UI_LibSprite(const char* name = "") : StringTableBase(name) {}
	void serialize(Archive& ar);
};

typedef StringTable<UI_LibSprite> UI_SpriteLibrary;
typedef StringTableReference<UI_LibSprite, false> UI_SpriteReference;

class UI_UnitSprite : public UI_Sprite
{
public:
	UI_UnitSprite();
	~UI_UnitSprite();

	void serialize(Archive& ar);

	Vect2f size() const;
	bool draw(const Vect3f& pos, float zShift, const Color4f& diffuse, const Vect2f& slot = Vect2f::ZERO, float scale = 1.f, Recti* posOut = 0) const;

private:
	bool useLegionColor_;

	Vect2f spriteOffset_;
	float spriteScale_;
	float maxPerspectiveScale_;
	float minPerspectiveScale_;
	float hideScale_;
};

#endif //__UI_SPRITE_H__