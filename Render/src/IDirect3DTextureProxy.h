#ifndef _IDIRECT3DTEXTUREPROXY_H_
#define _IDIRECT3DTEXTUREPROXY_H_

struct IDirect3DTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

//#define _USETEXTURE_PROXY_
#ifndef _USETEXTURE_PROXY_
typedef IDirect3DTexture9 IDirect3DTextureProxy;

#define GETIDirect3DTexture(p) p
#define GETIDirect3DTexturePassLock(p) p
#define CREATEIDirect3DTextureProxy(p) p

typedef IDirect3DVertexBuffer9 IDirect3DVertexBufferProxy;

#define GETIDirect3DVertexBuffer(p) p
#define CREATEIDirect3DVertexBufferProxy(p) p

typedef IDirect3DIndexBuffer9 IDirect3DIndexBufferProxy;

#define GETIDirect3DIndexBuffer(p) p
#define CREATEIDirect3DIndexBufferProxy(p) p

#else
#include <d3d9.h>
#define GETIDirect3DTexture(p) p->Get()
#define GETIDirect3DTexturePassLock(p) p->GetPassLock()

#define CREATEIDirect3DTextureProxy(p) new IDirect3DTextureProxy(p)

class IDirect3DTextureProxy: public cUnknownClass
{
protected:
	IDirect3DTexture9* texture;
	bool is_locked;

	struct SLevel
	{
		D3DSURFACE_DESC desc;
		BYTE* buffer;
		int buffer_size;
		int pixel_size;
		RECT rect;
		DWORD Flags;
	};
	vector<SLevel> levels;
public:
	IDirect3DTextureProxy(IDirect3DTexture9* p);
	~IDirect3DTextureProxy();

	HRESULT AddDirtyRect(CONST RECT * pDirtyRect);
	HRESULT GetLevelDesc(UINT Level,D3DSURFACE_DESC * pDesc);
	HRESULT GetSurfaceLevel(UINT Level,IDirect3DSurface9 ** ppSurfaceLevel);

	HRESULT LockRect(UINT Level,D3DLOCKED_RECT * pLockedRect,CONST RECT * pRect,DWORD Flags);
	HRESULT UnlockRect(UINT Level);

	bool IsLocked()const{return is_locked;}

	IDirect3DTexture9* Get(){xassert(!is_locked);return texture;}
	IDirect3DTexture9* GetPassLock(){return texture;}

	//IDirect3DBaseTexture9
	DWORD GetLevelCount();
};

#define GETIDirect3DVertexBuffer(p) p->Get()
#define CREATEIDirect3DVertexBufferProxy(p) new IDirect3DVertexBufferProxy(p)

struct IDirect3DVertexBufferProxy: public cUnknownClass
{
protected:
	IDirect3DVertexBuffer9* buffer;
	D3DVERTEXBUFFER_DESC desc;
	BYTE* shadow_buffer;

	bool is_locked;
	UINT OffsetToLock;
	UINT SizeToLock;
	DWORD lock_flags;

public:
	IDirect3DVertexBufferProxy(IDirect3DVertexBuffer9* p);
	~IDirect3DVertexBufferProxy();

	HRESULT GetDesc(D3DVERTEXBUFFER_DESC * pDesc);
	HRESULT Lock(UINT OffsetToLock,UINT SizeToLock,VOID ** ppbData,DWORD Flags);
	HRESULT Unlock();

	IDirect3DVertexBuffer9* Get(){xassert(!is_locked);return buffer;}
};

#define GETIDirect3DIndexBuffer(p) p->Get()
#define CREATEIDirect3DIndexBufferProxy(p) new IDirect3DIndexBufferProxy(p)

struct IDirect3DIndexBufferProxy: public cUnknownClass
{
protected:
	IDirect3DIndexBuffer9* buffer;
	BYTE* shadow_buffer;
	int num_polygon;
	D3DINDEXBUFFER_DESC desc;
	bool is_locked;
	UINT OffsetToLock;
	UINT SizeToLock;
	DWORD lock_flags;
public:
	IDirect3DIndexBufferProxy(IDirect3DIndexBuffer9* p);
	~IDirect3DIndexBufferProxy();
	HRESULT GetDesc(D3DINDEXBUFFER_DESC * pDesc);
	HRESULT Lock(UINT OffsetToLock,UINT SizeToLock,VOID ** ppbData,DWORD Flags);
	HRESULT Unlock();
	IDirect3DIndexBuffer9* Get(){xassert(!is_locked);return buffer;}
};
			

#endif _USETEXTURE_PROXY_

#endif _IDIRECT3DTEXTUREPROXY_H_
