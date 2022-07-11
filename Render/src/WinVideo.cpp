#include "StdAfxRD.h"
#include "WinVideo.h"
#include "D3DRender.h"
#include <amstream.h>	// DirectShow multimedia stream interfaces
#include <control.h>
#include <uuids.h>
#include <evcode.h>

#if (_MSC_VER < 1300)
#pragma comment(lib,"strmbase")
#else
#pragma comment(lib,"strmiids")
#endif

void sWinVideo::Init()
{
	CoInitialize(0);
}
void sWinVideo::Done()
{
	CoUninitialize();
}
void sWinVideo::SetWin(void *hWnd,int x,int y,int xsize,int ysize)
{
	sWinVideo::hWnd=hWnd;
	if(pVideoWindow&&hWnd) 
	{	//Set the video window.
		pVideoWindow->put_Owner((OAHWND)hWnd);
		pVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS);

		RECT grc={x,y,xsize,ysize};
		if(grc.right==0||grc.bottom==0) GetClientRect((HWND)hWnd, &grc);
		pVideoWindow->SetWindowPosition(grc.left,grc.top,grc.right,grc.bottom);
	}
}
int sWinVideo::Open(char *fname)
{
	sWinVideo::Close();
    // Create the filter graph manager.
    CoCreateInstance(CLSID_FilterGraph, 0,CLSCTX_INPROC,IID_IGraphBuilder,(void**)&pGraphBuilder);
	if(pGraphBuilder==0) { Close(); return 1; }
    pGraphBuilder->QueryInterface(IID_IMediaControl,(void**)&pMediaControl);
	if(pMediaControl==0) { Close(); return 2; }
    pGraphBuilder->QueryInterface(IID_IVideoWindow,(void**)&pVideoWindow);
	if(pVideoWindow==0)  { Close(); return 3; }
	pGraphBuilder->QueryInterface(IID_IMediaEvent,(void**)&pMediaEvent);
	if(pMediaEvent==0)  { Close(); return 3; }
    WCHAR wPath[MAX_PATH]; // Convert the file name to a wide-character string.
    MultiByteToWideChar(CP_ACP, 0, fname, -1, wPath, MAX_PATH);    
    if(pGraphBuilder->RenderFile(wPath,0)) { Close(); return 4; }
	if(hWnd) SetWin(hWnd);
	return 0;
}
void sWinVideo::Play()
{	// Run the graph.
	if(pGraphBuilder&&pVideoWindow&&pMediaControl&&hWnd) 
		pMediaControl->Run();
}
void sWinVideo::Stop()
{	// stop the graph.
	if(pGraphBuilder&&pVideoWindow&&pMediaControl&&hWnd) 
		pMediaControl->Stop();
}
int sWinVideo::IsPlay()
{
	if(pGraphBuilder&&pVideoWindow&&pMediaControl&&hWnd) 
	{
		OAFilterState pfs;
		if(pMediaControl->GetState(INFINITE,&pfs)!=S_OK) 
			return 0;
		if(pfs==State_Running) return 1;
		if(pfs==State_Stopped) return 0;
		if(pfs==State_Paused) return -1;
	}
	return 0;
};
void sWinVideo::WaitEnd()
{
    long evCode;
    if(pMediaEvent)
		pMediaEvent->WaitForCompletion(INFINITE,&evCode);
}
int sWinVideo::IsComplete()
{
    long evCode,param1,param2;
	while(pMediaEvent->GetEvent(&evCode,&param1,&param2,0)==S_OK)
	{
		pMediaEvent->FreeEventParams(evCode,param1,param2);
		if((EC_COMPLETE==evCode)||(EC_USERABORT==evCode))
			return 1;
	}
	return 0;
}
void sWinVideo::FullScreen(int bFullScreen)
{
	if(pVideoWindow)	
		pVideoWindow->put_FullScreenMode(bFullScreen);
}
void sWinVideo::HideCursor(int hide)
{
	if(pVideoWindow==0)	return;
	pVideoWindow->HideCursor(hide); 
}
void sWinVideo::GetSize(int *xsize,int *ysize)
{
	if(pVideoWindow==0)	return;
	pVideoWindow->get_Width((long*)xsize); 
	pVideoWindow->get_Height((long*)ysize);
}
void sWinVideo::SetSize(int xsize,int ysize)
{
	if(pVideoWindow==0)	return;
	pVideoWindow->put_Width(xsize); 
	pVideoWindow->put_Height(ysize);
}
void sWinVideo::Close()
{
	if(pMediaEvent)
	{
		pMediaEvent->Release();
		pMediaEvent=0;
	}
    if(pVideoWindow) 
	{
		pVideoWindow->put_Visible(FALSE);
//		pVideoWindow->put_Owner(0);   // может произойти потеря фокуса в полноэкранном режиме
	}
    if(pMediaControl)
	{
		pMediaControl->Release(); 
		pMediaControl=0;
	}
    if(pVideoWindow) 
	{
		pVideoWindow->Release(); 
		pVideoWindow=0;
	}
    if(pGraphBuilder)
	{
		pGraphBuilder->Release(); 
		pGraphBuilder=0;
	}
}


////////////////////////sVideoWrite//////////////////////////////
sVideoWrite::sVideoWrite(bool use_rgb_format_)
{
    pf=0;
    psSmall=0;
	iCurStreamPos=0;
	pRenderTexture=0;
	pSysSurface=0;
	stream_is_open=false;
	use_rgb_format=use_rgb_format_;
}

sVideoWrite::~sVideoWrite()
{
	Close();
}

void sVideoWrite::Close()
{
	RELEASE(pSysSurface);
	RELEASE(pRenderTexture);
	if(stream_is_open)
	{
		AVIStreamRelease(psSmall); 
		AVIFileRelease(pf);
	}
	stream_is_open=false;
}

Vect2i sVideoWrite::GetSize()
{
	return size;
}

bool sVideoWrite::Open(const char* file_name,int sizex,int sizey,int frame_rate)
{
	Close();
	size.x=sizex;
	size.y=sizey;
	AVISTREAMINFO    strhdr; 
	memset(&strhdr,0,sizeof(strhdr));
	strhdr.fccType=streamtypeVIDEO;
    strhdr.fccHandler=mmioStringToFOURCC("DIB", 0);
    strhdr.dwFlags=0; 
    strhdr.dwCaps=0; 
    strhdr.wPriority=0;
    strhdr.wLanguage=0;
/*
    strhdr.dwScale=1; 
    strhdr.dwRate=0x17;
    strhdr.dwStart=0;
    strhdr.dwLength=0x17; 
/*/
    strhdr.dwScale=1; 
    strhdr.dwRate=frame_rate;
    strhdr.dwStart=0;
    strhdr.dwLength=30; 
/**/
    strhdr.dwInitialFrames=0;
    strhdr.dwSuggestedBufferSize=0;
    strhdr.dwQuality=-1;
    strhdr.dwSampleSize=0; 
    strhdr.dwEditCount=0; 
    strhdr.dwFormatChangeCount=0; 

	strhdr.rcFrame.left=strhdr.rcFrame.top=0;
	strhdr.rcFrame.right=sizex;
	strhdr.rcFrame.bottom=sizey;

    strcpy(strhdr.szName,"K-D"); 
	iCurStreamPos=0;

	HRESULT hr;
    hr = AVIFileOpen(&pf, file_name, OF_WRITE | OF_CREATE, 0); 
    if (hr != 0) 
        return false; 
	stream_is_open=true;

	memset(&bi,0,sizeof(bi));
	bi.biSize=sizeof(bi);
	bi.biWidth=sizex;
	bi.biHeight=-sizey;
	bi.biPlanes=1;
	if(use_rgb_format)
	{
		bi.biBitCount=24;
		bi.biCompression=BI_RGB;
	}else
	{
		bi.biBitCount=32;
		bi.biCompression=BI_RGB;
	}

	bi.biSizeImage = ((((UINT)bi.biBitCount * size.x 
                        + 31)&~31) / 8) * size.y; 

	bi.biXPelsPerMeter=0;
	bi.biYPelsPerMeter=0;
	bi.biClrUsed=0;
	bi.biClrImportant=0;

	bitmapsize=bi.biSizeImage;

	SetRect(&strhdr.rcFrame, 0, 0, size.x, 
            size.y); 

   hr = AVIFileCreateStream(pf, &psSmall, &strhdr); 
    if (hr != 0)
	{
        AVIFileRelease(pf); 
		pf=0;
		stream_is_open=false;
        return false;
    } 

	hr = AVIStreamSetFormat(psSmall, 0, &bi, sizeof(bi)); 
    if (hr != 0) { 
        AVIStreamRelease(psSmall); 
        AVIFileRelease(pf); 
		psSmall=0;
		pf=0;
		stream_is_open=false;
        return false;
    } 

	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateTexture(size.x,size.y,1,D3DUSAGE_RENDERTARGET,D3DFMT_X8R8G8B8,D3DPOOL_DEFAULT,&pRenderTexture,0));
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateOffscreenPlainSurface(size.x,size.y,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&pSysSurface,0));

	return true;
}

bool sVideoWrite::WriteFrame()
{
	if(!stream_is_open)
		return false;
	RECT rc;
	SetRect(&rc, 0, 0, size.x, size.y); 

	IDirect3DSurface9 *pDestSurface=0;
	RDCALL(pRenderTexture->GetSurfaceLevel(0,&pDestSurface));

	RDCALL(gb_RenderDevice3D->D3DDevice_->StretchRect(
		gb_RenderDevice3D->backBuffer_,0,pDestSurface,&rc,D3DTEXF_LINEAR));

	RDCALL(gb_RenderDevice3D->D3DDevice_->GetRenderTargetData(pDestSurface,pSysSurface));

//	HRESULT GetRenderTargetData(gb_RenderDevice3D->lpBackBuffer,
//    IDirect3DSurface9* pDestSurface
//);

//	BYTE* lpNew=;
	D3DLOCKED_RECT LockedRect;
	RDCALL(pSysSurface->LockRect(&LockedRect,
		0,
		D3DLOCK_READONLY
	));

	if(use_rgb_format)
		Move4To3((char*)LockedRect.pBits,bi.biSizeImage/3);

	bool ok=
	SUCCEEDED(AVIStreamWrite(psSmall, iCurStreamPos, 1, LockedRect.pBits,
            bi.biSizeImage, AVIIF_KEYFRAME, 0, 0));

	pSysSurface->UnlockRect();

	pDestSurface->Release();
	iCurStreamPos++;
	return ok;
}

void sVideoWrite::Move4To3(char* buffer,int num)
{
	char* out=buffer;
	for(int i=0;i<num;i++)
	{
		*out++=*buffer++;
		*out++=*buffer++;
		*out++=*buffer++;
		buffer++;
	}
}
