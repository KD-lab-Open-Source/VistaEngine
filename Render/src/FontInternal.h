#ifndef __FONT_INTERNAL_H_INCLUDED__
#define __FONT_INTERNAL_H_INCLUDED__

class cFontInternal : public cUnknownClass
{
public:
	cFontInternal();
	virtual ~cFontInternal();

	bool Create(LPCSTR fname,int h,bool silentErr=0);
	bool Create(void* pFontData,int sizefontdata,int h,bool silentErr=0);
	
	virtual cTexture* GetTexture() const	{return pTexture;};
	virtual float GetHeight()	const		{return FontHeight*GetTexture()->GetHeight();}
	bool Reload();

	vector<Vect3f>		Font; // x,y - position, z - font width
	float				FontHeight;

	string font_name;
	int GetStatementHeight(){return statement_height;};

	void* GetFontData()const{return pFontData;}
protected:
	void* pFontData;
	int sizefontdata;
	int statement_height;
	cTexture* pTexture;
	friend class cFont;
	
	bool CreateTexture(LPCSTR fontname,LPCSTR fname,int height);
	bool CreateImage(LPCSTR filename,LPCSTR fontname,int height,class cFontImage* image);
	bool Save(const char* fname,cFontImage& fnt);
	bool Load(const char* fname,cFontImage& fnt);
};


#endif
