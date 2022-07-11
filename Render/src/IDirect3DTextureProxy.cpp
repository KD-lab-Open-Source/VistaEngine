#include "StdAfxRd.h"
#include "IDirect3DTextureProxy.h"
#ifdef _USETEXTURE_PROXY_

IDirect3DTextureProxy::IDirect3DTextureProxy(IDirect3DTexture9* p):texture(p)
{
	is_locked=false;
	levels.resize(texture->GetLevelCount());
	for(int level=0;level<levels.size();level++)
	{
		SLevel& s=levels[level];
		RDCALL(texture->GetLevelDesc(level,&s.desc));
		s.pixel_size=GetTextureFormatSize(s.desc.Format)/8;
		s.buffer=NULL;
		s.rect.left=s.rect.right=s.rect.top=s.rect.bottom=0;
		s.Flags=0;
	}
}

IDirect3DTextureProxy::~IDirect3DTextureProxy()
{
	RELEASE(texture);
	for(int level=0;level<levels.size();level++)
	{
		delete levels[level].buffer;
	}
}

HRESULT IDirect3DTextureProxy::AddDirtyRect(CONST RECT * pDirtyRect)
{
	xassert(0);
	return texture->AddDirtyRect(pDirtyRect);
}

HRESULT IDirect3DTextureProxy::GetLevelDesc(UINT Level,D3DSURFACE_DESC * pDesc)
{
	return texture->GetLevelDesc(Level,pDesc);
}

HRESULT IDirect3DTextureProxy::GetSurfaceLevel(UINT Level,IDirect3DSurface9 ** ppSurfaceLevel)
{
	return texture->GetSurfaceLevel(Level,ppSurfaceLevel);
}

HRESULT IDirect3DTextureProxy::LockRect(UINT Level,D3DLOCKED_RECT * pLockedRect,CONST RECT * pRect,DWORD Flags)
{
	xassert(!is_locked);
	is_locked=true;

	SLevel& s=levels[Level];
	if(pRect)
	{
		s.rect=*pRect;
	}else
	{
		s.rect.left=0;
		s.rect.top=0;
		s.rect.right=s.desc.Width;
		s.rect.bottom=s.desc.Height;
	}

	xassert(s.rect.left>=0);
	xassert(s.rect.top>=0);
	xassert(s.rect.left<=s.rect.right);
	xassert(s.rect.top<=s.rect.bottom);
	xassert(s.rect.right<=s.desc.Width);
	xassert(s.rect.bottom<=s.desc.Height);

	s.Flags=Flags;
	s.buffer_size=(s.rect.right-s.rect.left)*(s.rect.bottom-s.rect.top)*s.pixel_size+4;
	s.buffer=new BYTE[s.buffer_size];
	*(DWORD*)(s.buffer+s.buffer_size-4)=0xc0dedead;
	pLockedRect->Pitch=(s.rect.right-s.rect.left)*s.pixel_size;
	pLockedRect->pBits=s.buffer;
	return S_OK;
}
HRESULT IDirect3DTextureProxy::UnlockRect(UINT Level)
{
	xassert(is_locked);
	is_locked=false;
	SLevel& s=levels[Level];

	xassert(*(DWORD*)(s.buffer+s.buffer_size-4)==0xc0dedead);

	D3DLOCKED_RECT lock_rect;
	RECT in;
	in.left=0;
	in.top=0;
	in.right=s.rect.right-s.rect.left;
	in.bottom=s.rect.bottom-s.rect.top;
	int inPitch=in.right*s.pixel_size;
	RDCALL(texture->LockRect(Level,&lock_rect,&s.rect,s.Flags));
	
	int dy=in.bottom;
	for(int y=0;y<dy;y++)
	{
		memcpy(lock_rect.Pitch*y+(BYTE*)lock_rect.pBits,s.buffer+inPitch*y,inPitch);
	}

	delete s.buffer;
	s.buffer=NULL;
	return texture->UnlockRect(Level);
}


DWORD IDirect3DTextureProxy::GetLevelCount()
{
	return texture->GetLevelCount();
}

static DWORD vertex_mask=0x7f865431L;
const vertex_additional=64;

IDirect3DVertexBufferProxy::IDirect3DVertexBufferProxy(IDirect3DVertexBuffer9* p)
:buffer(p)
{
	RDCALL(buffer->GetDesc(&desc));
	is_locked=false;
	lock_flags=0;
	OffsetToLock=0;
	SizeToLock=0;

	shadow_buffer=new BYTE[desc.Size+vertex_additional];
	xassert(!(desc.Size&3));
	for(int i=0;i<desc.Size+vertex_additional;i+=4)
		*(DWORD*)(shadow_buffer+i)=vertex_mask;
}

IDirect3DVertexBufferProxy::~IDirect3DVertexBufferProxy()
{
	RELEASE(buffer);
	delete shadow_buffer;
}

HRESULT IDirect3DVertexBufferProxy::GetDesc(D3DVERTEXBUFFER_DESC * pDesc)
{
	return buffer->GetDesc(pDesc);
}

HRESULT IDirect3DVertexBufferProxy::Lock(UINT OffsetToLock_,UINT SizeToLock_,VOID ** ppbData,DWORD Flags)
{
	xassert(!is_locked);
	is_locked=true;
	OffsetToLock=OffsetToLock_;
	SizeToLock=SizeToLock_;
	lock_flags=Flags;
	if(SizeToLock==0)
		SizeToLock=desc.Size-OffsetToLock;

	xassert(OffsetToLock<=desc.Size);
	xassert(OffsetToLock+SizeToLock<=desc.Size);

	*ppbData=((BYTE*)shadow_buffer)+OffsetToLock;
	return S_OK;
}

HRESULT IDirect3DVertexBufferProxy::Unlock()
{
	xassert(is_locked);
	is_locked=false;
	if(lock_flags&D3DLOCK_READONLY)
		return S_OK;
	VOID *pbData=NULL;
	RDCALL(buffer->Lock(OffsetToLock,SizeToLock,&pbData,lock_flags));
//	for(int i=OffsetToLock;i<OffsetToLock+SizeToLock;i+=4)
//		xassert(*(DWORD*)(shadow_buffer+i)!=vertex_mask);
	for(int i=desc.Size;i<desc.Size+vertex_additional;i+=4)
		xassert(*(DWORD*)(shadow_buffer+i)==vertex_mask);
	//memcpy(pbData,((BYTE*)shadow_buffer)+OffsetToLock,SizeToLock);
	{
		for(int i=0;i<SizeToLock;i+=4)
		{
			DWORD in=*(DWORD*)(((BYTE*)shadow_buffer)+OffsetToLock+i);
			*(DWORD*)(i+(BYTE*)pbData)=in;
		}
	}

	return buffer->Unlock();
}

static sPolygon end;
IDirect3DIndexBufferProxy::IDirect3DIndexBufferProxy(IDirect3DIndexBuffer9* p)
:buffer(p)
{
	is_locked=false;
	lock_flags=0;
	OffsetToLock=0;
	SizeToLock=0;
	RDCALL(buffer->GetDesc(&desc));
	num_polygon=desc.Size/sizeof(sPolygon);
	xassert(num_polygon*sizeof(sPolygon)==desc.Size);
	shadow_buffer=new BYTE[desc.Size+sizeof(sPolygon)];

	end.p1=end.p2=end.p3=0xdead;
	((sPolygon*)shadow_buffer)[num_polygon]=end;
}

IDirect3DIndexBufferProxy::~IDirect3DIndexBufferProxy()
{
	delete shadow_buffer;
	RELEASE(buffer);
}

HRESULT IDirect3DIndexBufferProxy::GetDesc(D3DINDEXBUFFER_DESC * pDesc)
{
	return buffer->GetDesc(pDesc);
}

HRESULT IDirect3DIndexBufferProxy::Lock(UINT OffsetToLock_,UINT SizeToLock_,VOID ** ppbData,DWORD Flags)
{
	xassert(!is_locked);
	is_locked=true;
	if(SizeToLock_==0)
		SizeToLock_=desc.Size;
	OffsetToLock=OffsetToLock_;
	SizeToLock=SizeToLock_;
	lock_flags=Flags;

	xassert(OffsetToLock%sizeof(sPolygon)==0);
	xassert(SizeToLock%sizeof(sPolygon)==0);
	xassert(OffsetToLock>=0);
	xassert(OffsetToLock+SizeToLock<=desc.Size);
	*ppbData=shadow_buffer+OffsetToLock;

	return S_OK;
}

HRESULT IDirect3DIndexBufferProxy::Unlock()
{
	xassert(is_locked);
	is_locked=false;
	sPolygon& e=((sPolygon*)shadow_buffer)[num_polygon];
	xassert(e.p1==end.p1 && e.p2==end.p2 && e.p3==end.p3);
	if(lock_flags&D3DLOCK_READONLY)
		return S_OK;
	BYTE* pData;
	RDCALL(buffer->Lock(OffsetToLock,SizeToLock,(void**)&pData,lock_flags));
	memcpy(pData,shadow_buffer+OffsetToLock,SizeToLock);
	return buffer->Unlock();
}

#endif _USETEXTURE_PROXY_
