#include "StdAfx.h"
#define D3D_DEBUG_INFO
#include <d3d9.h>
#include <d3dx9.h>
#include "PlayBink.h"

#include "Sound.h"
#include "SystemUtil.h"
#include "RenderObjects.h"
#include "SoundApp.h"
#include "PlayOgg.h"
#include "Render\3dx\umath.h"
#include "Render\Src\VisGeneric.h"
#include "Render\Src\Texture.h"
#include "Render\Inc\IRenderDevice.h"

#ifdef _DEMO_
#define _NO_BINK_
#endif

#ifdef _NO_BINK_

class PlayBinkR : public PlayBink
{
public:
	PlayBinkR(){};
	~PlayBinkR(){}

	bool Init(const char* bink_name){}
	void Draw(int x,int y,int dx,int dy, int alpha = 255){}
	bool CalcNextFrame(){return false;}
	bool IsEnd(){return true;}
	Vect2i GetSize(){return Vect2i(10,10);}
	void SetVolume(float vol){}
};

#else

#pragma comment(lib,"binkw32")

class PlayBinkR : PlayBink
{
public:
	PlayBinkR();
	virtual ~PlayBinkR();

	bool Init(const char* bink_name);
	void Draw(int x,int y,int dx,int dy, int alpha = 255);
	bool CalcNextFrame();
	bool IsEnd();
	void SetVolume(float vol);
protected:
	bool focus;
	cTexture* pTextureBink;

	void ShowNextFrame();
	void Done();
};

//############################################################################
//##                                                                        ##
//##  Good_sleep_us - sleeps for a specified number of MICROseconds.        ##
//##    The task switcher in Windows has a latency of 15 ms.  That means    ##
//##    you can ask for a Sleep of one millisecond and actually get a       ##
//##    sleep of 15 ms!  In normal applications, this is no big deal,       ##
//##    however, with a video player at 30 fps, 15 ms is almost half our    ##
//##    frame time!  The Good_sleep_us function times each sleep and keeps  ##
//##    the average sleep time to what you requested.  It also give more    ##
//##    accuracy than Sleep - Good_sleep_us() uses microseconds instead of  ##
//##    milliseconds.                                                       ##
//##                                                                        ##
//############################################################################

static void Good_sleep_us( int microseconds )
{
  static int total_sleep=0;
  static int slept_in_advance=0;

  total_sleep += microseconds;

  //
  // Have we exceeded our reserve of slept microseconds?
  //

  if (( total_sleep - slept_in_advance ) > 1000)
  {
    int start, end;
    total_sleep -= slept_in_advance;

    //
    // Do the timed sleep.
    //

	start = xclock();
    Sleep( total_sleep / 1000 );
	end = xclock();

    //
    // Calculate delta time in microseconds.
    //

    end = (end - start) * (int)1000;

    //
    // Keep track of how much extra we slept.
    //

    slept_in_advance = ( int )end - total_sleep;
    total_sleep %= 1000;
  }
}

PlayBinkR::PlayBinkR()
{
	focus=true;
	pTextureBink=NULL;
}

PlayBinkR::~PlayBinkR()
{
	Done();
}

bool PlayBinkR::Init(const char* bink_name)
{
	return false;

	Done();
	string error;
}

void PlayBinkR::Done()
{
	RELEASE(pTextureBink);
}

bool PlayBinkR::CalcNextFrame()
{
	if(focus && !applicationHasFocus())
	{//KillFocus
	}

	if(!focus && applicationHasFocus())
	{//SetFocus
	}
	focus=applicationHasFocus();
	int curTime = xclock();
	return false;
}

bool PlayBinkR::IsEnd()
{
	return true;
}

void PlayBinkR::ShowNextFrame()
{
}

void PlayBinkR::Draw(int x,int y,int dx,int dy, int alpha)
{
}

void PlayBinkR::SetVolume(float vol)
{
}

//////////////////////////////
PlayBinkSample::PlayBinkSample()
{
	pos.set(0,0);
	size.set(-1,-1);
	pTextureBackground=NULL;
}

PlayBinkSample::~PlayBinkSample()
{
	RELEASE(pTextureBackground);
}

extern OggPlayer gb_Music;

void PlayBinkSample::DoModal(const char* bink_name,const char* sound_name)
{
	pTextureBackground=gb_VisGeneric->CreateTextureScreen();
	if(!pTextureBackground)
	{
		xassertStr(0,"Cannot create background texture to play bink file");
		return;
	}
	RELEASE(pTextureBackground);

	if(sound_name)
		gb_Music.stop();
}

#endif 
