#ifndef __WINVIDEO_H__
#define __WINVIDEO_H__

struct IGraphBuilder;
struct IMediaControl;
struct IVideoWindow;
struct IMediaEvent;

struct sWinVideo
{
	sWinVideo()											{ pGraphBuilder=0; pMediaControl=0; pVideoWindow=0; pMediaEvent=0; hWnd=0; }
	~sWinVideo()										{ Close(); }
	static void Init();					// initilize DirectShow Lib
	static void Done();					// uninitilize DirectShow Lib
	// direct	
	void SetWin(void *hWnd,int x=0,int y=0,int xsize=0,int ysize=0);
	int Open(char *fname);				// if it's all OK, then function return 0
	void Play();
	void Stop();
	void Close();
	void WaitEnd();
	int IsComplete();
	// util	
	void HideCursor(int hide=1);		// 1 - hide cursor, 0 - unhide cursor
	void GetSize(int *xsize,int *ysize);
	void SetSize(int xsize,int ysize);
	void FullScreen(int bFullScreen=1);	// 1 - FullScreen, 0 - Window
	int IsPlay();

private:
	IGraphBuilder	*pGraphBuilder;
	IMediaControl	*pMediaControl;
	IVideoWindow	*pVideoWindow;
    IMediaEvent		*pMediaEvent;
	void			*hWnd;
};

#include <vfw.h>		// AVI include

struct sVideoWrite
{
public:
	sVideoWrite(bool use_rgb_format=true);
	~sVideoWrite();
	bool Open(const char* file_name,int sizex,int sizey,int frame_rate=30);
	void Close();
	bool WriteFrame();

	DWORD GetDataSize(){return (DWORD)bitmapsize*(DWORD)iCurStreamPos;}

	Vect2i GetSize();
protected:
	Vect2i size;
	bool use_rgb_format;
	bool stream_is_open;
    PAVIFILE         pf; 
    PAVISTREAM       psSmall; 
	BITMAPINFOHEADER bi; 
	int bitmapsize;
	int iCurStreamPos;
	IDirect3DTexture9* pRenderTexture;
	IDirect3DSurface9 *pSysSurface;

	void Move4To3(char* buffer,int num);
};

#endif //__WINVIDEO_H__
