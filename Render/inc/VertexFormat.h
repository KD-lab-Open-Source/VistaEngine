#ifndef __VERTEX_FORMAT_H_INCLUDED__
#define __VERTEX_FORMAT_H_INCLUDED__

#include "Render\3dx\umath.h"
#include "XMath\Colors.h"

struct IDirect3DVertexDeclaration9;

#pragma pack(push,1)
/// Чтобы полнее понимать ворматы - см. хэлп DirectX

struct RENDER_API sVertexXYZ
{
	Vect3f	pos;/// положение объекта в трехмерном пространстве
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZD : public sVertexXYZ
{
	Color4c	diffuse;
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZT1: public sVertexXYZ
{ 
	float			uv[2];/// кооридинаты на текстуре0
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	inline Vect2f& GetTexel()			{ return *((Vect2f*)&uv[0]); }
	inline Vect2f& GetTexel1()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZDT1 : public sVertexXYZD
{
	float			uv[2];/// кооридинаты на текстуре1
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	inline Vect2f& GetTexel()			{ return *((Vect2f*)&uv[0]); }
	inline Vect2f& GetTexel1()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZDT2 : public sVertexXYZDT1
{
	float			uv2[2];/// кооридинаты на текстуре2
	inline float& u2()					{ return uv2[0]; }
	inline float& v2()					{ return uv2[1]; }
	inline Vect2f& GetTexel2()			{ return *((Vect2f*)&uv2[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZDS : public sVertexXYZD
{
	Color4c	specular;
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZDST1 : public sVertexXYZDS
{
	float			uv[2];
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	inline Vect2f& GetTexel()			{ return *((Vect2f*)&uv[0]); }
	inline Vect2f& GetTexel1()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZDST2 : public sVertexXYZDST1
{
	float			uv[2];
	inline float& u2()					{ return uv[0]; }
	inline float& v2()					{ return uv[1]; }
	inline Vect2f& GetTexel2()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZWT1
{
	float			x,y,z,w;/// преобразованное в двухмерное пространство положение объекта. С такими форматами не применяется вертекс шейдер.
	float			uv[2];
	Vect3f& GetVect3f()					{ return *(Vect3f*)&x; }
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	inline Vect2f& GetTexel()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZW
{
	float			x,y,z,w;
	inline int& xi()					{ return *((int*)&x); }
	inline int& yi()					{ return *((int*)&y); }
	inline int& zi()					{ return *((int*)&z); }
	Vect3f& GetVect3f()					{ return *(Vect3f*)&x; }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZWD : public sVertexXYZW
{
	Color4c	diffuse;
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZWDT1 : public sVertexXYZWD
{
	float			uv[2];
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	inline Vect2f& GetTexel()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZWDT2 : public sVertexXYZWDT1
{
	float			uv[2];
	inline float& u2()					{ return uv[0]; }
	inline float& v2()					{ return uv[1]; }
	inline Vect2f& GetTexel2()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZWDT3 : public sVertexXYZWDT2
{
	float			uv[2];
	inline float& u3()					{ return uv[0]; }
	inline float& v3()					{ return uv[1]; }
	inline Vect2f& GetTexel3()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZWDT4 : public sVertexXYZWDT3
{
	float			uv[2];
	inline float& u4()					{ return uv[0]; }
	inline float& v4()					{ return uv[1]; }
	inline Vect2f& GetTexel4()			{ return *((Vect2f*)&uv[0]); }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZN : public sVertexXYZ
{
	Vect3f	n;/// нормаль
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZND : public sVertexXYZN
{
	Color4c	diffuse;
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZNDS : public sVertexXYZND
{
	Color4c	specular;
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZNDST1 : public sVertexXYZNDS
{
	float			uv[2];
	inline Vect2f& GetTexel()			{ return *(Vect2f*)&uv[0]; }
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZNDST2 : public sVertexXYZNDST1
{
	float			uv[2];
	inline float& u2()					{ return uv[0]; }
	inline float& v2()					{ return uv[1]; }
	inline Vect2f& GetTexel2()			{ return *(Vect2f*)&uv[0]; }
	static IDirect3DVertexDeclaration9* declaration;
};
struct RENDER_API sVertexXYZNT1 : public sVertexXYZN
{
	float			uv[2];
	inline Vect2f& GetTexel()			{ return *(Vect2f*)&uv[0]; }
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	static IDirect3DVertexDeclaration9* declaration;
};

struct RENDER_API sVertexXYZINT1 : public sVertexXYZ
{
	BYTE index[4];
	Vect3f	n;
	float			uv[2];
	inline Vect2f& GetTexel()			{ return *(Vect2f*)&uv[0]; }
	inline float& u1()					{ return uv[0]; }
	inline float& v1()					{ return uv[1]; }
	static IDirect3DVertexDeclaration9* declaration;
};

class RENDER_API cSkinVertex
{
/*плавающий формат для скининга с 1..4 костями и поддержкой бампа.
  как веса так и индексы матриц весов хранятся в виде байтиков.
	Vect3f	pos;
	BYTE index[4];
	Vect3f	n;
	BYTE weight[4];
	float			uv[2];
*/
	int num_weight;
	bool bump;
	bool uv2;
	bool fur;
	int vb_size;
	void* p;
	float* cur;
	int offset_texel;
	int offset_bump_s;
	int offset_bump_t;
	int offset_texel2;
	int offset_fur;
public:
	static IDirect3DVertexDeclaration9* declaration[2][2][2][2];

	cSkinVertex(int num_weight_,bool bump_,bool uv2_,bool fur_);
	void SetVB(void* p_,int vb_size_){p=p_;vb_size=vb_size_;}
	void Select(int n){cur=(float*)(vb_size*n+(char*)p);}
	Vect3f& GetPos(){return *(Vect3f*)cur;}
	Color4c& GetIndex(){return *(Color4c*)(3+cur);}
	Vect3f& GetNorm(){return *(Vect3f*)(4+cur);}
	BYTE& GetWeight(int idx);
	Vect2f& GetTexel(){return *(Vect2f*)(offset_texel+cur);}

	/// S,T,SxT=N - матрица преобразования в пространство треугольника для бампа.
	Vect3f& GetBumpS(){return *(Vect3f*)(offset_bump_s+cur);}
	Vect3f& GetBumpT(){return *(Vect3f*)(offset_bump_t+cur);}

	Vect2f& GetTexel2(){return *(Vect2f*)(offset_texel2+cur);}

	Color4c& GetFur(){return *(Color4c*)(offset_fur+cur);}

	IDirect3DVertexDeclaration9* GetDeclaration();

	static void Register();
protected:
};
/*
Как можно экономить.
Было 4*3*4=48 байт на данных, которые реально сэкономить. GetPos, GetNorm, GetBumpS, GetBumpT
+ GetIndex, GetWeight, GetTexel -> 16 байт на которых нереально сэкономить
1) GetPos, GetNorm, GetBumpS, GetBumpT -> short -> 3*4*2=24 байта, 3 умножения + 3 копирования. (возможно копирования не нужны если аккуратно код написать).
2) GetTexel -> short -> 4 байта , +1 умножение.
3) GetNorm, GetBumpS, GetBumpT -> color -> 3*4=12 байт, нет дополнительных умножений, но точность низка, хотя и сопоставима с точностью карты нормалей.
4) T=cross(N,S) -> 2 операции, -12 байт
*/

struct sVertexD
{
	Color4c	diffuse;
	static IDirect3DVertexDeclaration9* declaration;
};

struct sShort2
{
	short x;
	short y;
};

struct sShort4
{
	short x;
	short y;
	short z;
	short w;
};

struct shortVertexXYZ
{
	sShort4 pos;
	static IDirect3DVertexDeclaration9* declaration;
};

struct shortVertexXYZD : public shortVertexXYZ
{
	Color4c	diffuse;
	static IDirect3DVertexDeclaration9* declaration;
};

struct shortVertexGrass : public shortVertexXYZD
{
	Color4c n;
	short uv[4];
	float uv2;
	static IDirect3DVertexDeclaration9* declaration;
};
#pragma pack(pop)

#endif
