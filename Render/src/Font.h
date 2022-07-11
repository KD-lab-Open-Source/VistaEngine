#ifndef _FONT_H_
#define _FONT_H_
//Фонт пытаются селектить в логическом потоке, нужно перенести функции узнать высоту и ширину в cFont
class cFont : public cUnknownClass
{
public:
	cFont(const cFontInternal* p) : pFont(p), scale_(1.f, 1.f) {};
	cFont(const cFontInternal* p, const Vect2f& scale) : pFont(p), scale_(scale) {};
	virtual ~cFont(){};

	virtual cTexture* GetTexture() const;
	virtual float GetHeight() const;

	virtual Vect2f GetScale() const { return scale_; }
	virtual void SetScale(const Vect2f& scale) { scale_ = scale; }

	inline const cFontInternal* GetInternal() const { return pFont; }

	float size() const;
  
	float GetLength(const char *string);
	float GetHeight(const char *string);
	float GetLineLength(const char *string);
	float GetCharLength(const char c);
protected:
	Vect2f scale_;
	const cFontInternal* pFont;
};

void ChangeTextColor(const char* &str,sColor4c& diffuse);

#endif _FONT_H_
