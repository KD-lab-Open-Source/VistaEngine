#ifndef __TEXTURE_INL_INCLUDED__
#define __TEXTURE_INL_INCLUDED__

enum eAttributeTexture
{
	TEXTURE_NONDELETE		=	1<<4,
	TEXTURE_DYNAMIC			=   1<<5,    //Текстура часто меняется
	TEXTURE_ALPHA_BLEND		=	1<<6,	//  текстура содержит альфу
	TEXTURE_DISABLE_DETAIL_LEVEL=1<<7, 
	TEXTURE_ALPHA_TEST		=	1<<8,	// текстура содержит маску в альфе
	TEXTURE_BUMP			=   1<<9,
	TEXTURE_NO_COMPACTED	=	1<<10,  // текстура не удаляется при вызове GetTexLibrary()->Compact();
	TEXTURE_SPECULAR		=	1<<11,
	TEXTURE_MIPMAP_POINT	=	1<<18,		// текстурные мипмапы получены ближайшими точками
	TEXTURE_MIPMAP_POINT_ALPHA=	1<<19,		// текстурные мипмапы получены ближайшими точками только для apha
	TEXTURE_R32F			=   1<<20,		// 32-bit float format 
	TEXTURE_RENDER16		=	1<<21,		// в текстуру происходит рендер
	TEXTURE_RENDER32		=	1<<22,
	TEXTURE_ADDED_POOL_DEFAULT	=1<<23,
	TEXTURE_32				=	1<<24,		//Только 32 битный формат
	TEXTURE_RENDER_SHADOW_9700 =1<<25,
	TEXTURE_D3DPOOL_DEFAULT =	1<<26,
	TEXTURE_GRAY			=   1<<27,
	TEXTURE_UVBUMP			=	1<<28,
	TEXTURE_U16V16			=	1<<29,
	TEXTURE_CUBEMAP			=	1<<30,
	TEXTURE_NODDS			=	1<<31,

};

inline bool cTexture::IsAlpha()
{
	return GetAttribute(TEXTURE_ALPHA_BLEND);
}

inline bool cTexture::IsAlphaTest()
{
	return GetAttribute(TEXTURE_ALPHA_TEST);
}

inline int cTexture::GetTimePerFrame()
{
	return TimePerFrame;
}

inline void cTexture::SetTimePerFrame(int tpf)
{
	TimePerFrame=tpf;
}

inline void cTexture::New(int number)						
{ 
	BitMap.resize(number); 
	for(unsigned i=0;i<BitMap.size();i++) 
		BitMap[i]=0; 
}
inline eSurfaceFormat cTexture::GetFmt()					
{ 
	if(format!=SURFMT_BAD)
		return format;
	if(GetAttribute(TEXTURE_GRAY) && GetAttribute(TEXTURE_ALPHA_BLEND))
		return SURFMT_L8;
	if(GetAttribute(TEXTURE_RENDER_SHADOW_9700))
		return SURFMT_RENDERMAP_FLOAT;
	if(GetAttribute(TEXTURE_RENDER16))
		return SURFMT_RENDERMAP16;
	if(GetAttribute(TEXTURE_RENDER32))
		return SURFMT_RENDERMAP32;
	if(GetAttribute(TEXTURE_32))
		return GetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST) ? SURFMT_COLORALPHA32 : SURFMT_COLOR32;
	if(GetAttribute(TEXTURE_BUMP))
		return SURFMT_BUMP;
	if(GetAttribute(TEXTURE_UVBUMP))
		return SURFMT_UV;
	if(GetAttribute(TEXTURE_U16V16))
		return SURFMT_U16V16;
	if(GetAttribute(TEXTURE_R32F))
		return SURFMT_R32F;

	return GetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST) ? SURFMT_COLORALPHA : SURFMT_COLOR;
}

#endif
