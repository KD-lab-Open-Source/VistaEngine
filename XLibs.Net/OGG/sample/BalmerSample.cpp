#include "stdafx.h"
#include <commctrl.h>

#include <dsound.h>
#include "..\PlayOgg\PlayOgg.h"
#include <xutil.h>
#include "resource.h"

inline float frand(){return rand()/(float)0x7fff;}

HWND g_hWnd=NULL;

void win_printf(char *format, ...)
{
  va_list args;
  char    buffer[512];

  va_start(args,format);

  vsprintf(buffer,format,args);

  MessageBox(g_hWnd,buffer,"Message...",MB_OK);
}


MpegSound* sounds=NULL;
MpegSound sound_xxx;


vector<char*> names;
int cur_name=0;


#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
LPDIRECTSOUND       g_pDS       = NULL;

HRESULT InitDirectSound(HWND g_hWnd)
{
    HRESULT             hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &g_pDS, NULL ) ) )
	{
		win_printf("Fail DirectSoundCreate");
        return hr;
	}

    // Set coop level to DSSCL_PRIORITY 
    if( FAILED( hr = g_pDS->SetCooperativeLevel( g_hWnd, DSSCL_PRIORITY ) ) )
	{
		win_printf("Fail SetCooperativeLevel");
        return hr;
	}
    
    // Get the primary buffer 
    DSBUFFERDESC        dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
       
    if( FAILED( hr = g_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
	{
		win_printf("Fail CreateSoundBuffer");
        return hr;
	}

	struct FORMATS
	{
		int cannels;
		int herz;
		int bits;
	} formats[]=
	{
		{1,22050,8},
		{2,22050,8},
		{1,22050,16},
		{2,22050,16},
		{1,44100,16},
		{2,44100,16},
	};

	for(int i=SIZE(formats)-1;i>=0;i--)
	{
		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
		wfx.wFormatTag      = WAVE_FORMAT_PCM; 
		wfx.nChannels       = formats[i].cannels; 
		wfx.nSamplesPerSec  = formats[i].herz; 
		wfx.wBitsPerSample  = formats[i].bits; 
		wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

		hr = pDSBPrimary->SetFormat(&wfx);
		if(!FAILED(hr))
			break;
	}

	if(FAILED(hr))
	{
		win_printf("Fail SetFormat");
		return hr;
	}

    SAFE_RELEASE( pDSBPrimary );

	MpegInitLibrary(g_pDS);
    return S_OK;
}

void ReleaseDirectSound()
{
	MpegDeinitLibrary();

    SAFE_RELEASE( g_pDS ); 
}


INT_PTR CALLBACK MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_INITDIALOG:
		{
			g_hWnd=hWnd;
			if(FAILED(InitDirectSound(g_hWnd)))
			{
				win_printf("InitDirectSound failed");
				return 1;
			}


			{
				sounds=new MpegSound;
/*
				if(!sounds->OpenToPlay(names[cur_name],true))
				{
					win_printf("Cannot open %s",names[cur_name]);
					delete sounds;
					sounds=NULL;
				}
*/
			}

			if(sounds)
			{
				HWND hSlider  = GetDlgItem( hWnd, IDC_SLIDER1 );

				int len=round(sounds->GetLen()*100);
				PostMessage( hSlider, TBM_SETRANGEMAX, TRUE, len);
				PostMessage( hSlider, TBM_SETRANGEMIN, TRUE, 0L );
			}


			SetTimer(hWnd,1,100,NULL);
			break;
		}
        case WM_COMMAND:
            switch( LOWORD(wParam) )
			{
			case IDOK:
				EndDialog( hWnd, IDOK);
				break;
			case IDCANCEL:
				EndDialog( hWnd, IDCANCEL);
				break;
			case IDC_BUTTON_SEEK:
				cur_name=(cur_name+1)%names.size();
				sounds->OpenToPlay(names[cur_name],false);
				break;
			case IDC_FADE:
				sounds->FadeVolume(1.0f,0);
				break;
			default:
				return FALSE;
			}
			break;
        case WM_DESTROY:
			ReleaseDirectSound();
            break;
		case WM_TIMER:
			{
				double us=MpegCPUUsing();
				char s[256];
				sprintf(s,"CPU=%2.3f%%",us*100);
				SetDlgItemText( hWnd,IDC_CPU_USING,s);

				if(sounds)
				{

					float pos=sounds->GetCurPos();
					sprintf(s,"%f",pos);
					SetDlgItemText(hWnd,IDC_CUR_SAMPLE,s);

					HWND hSlider  = GetDlgItem( hWnd, IDC_SLIDER1 );
					PostMessage( hSlider, TBM_SETPOS, TRUE, round(pos*100) );
				}
			}
			break;
		default:
			return FALSE;
    }
    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	names.push_back("001_T Belleville.ogg");
	names.push_back("Briefing_mission10_4.ogg");
	names.push_back("Briefing_mission10_2.ogg");
//	CoIninitialize();
	InitCommonControls();
	initclock();
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG_SOUND),NULL,MsgProc);

    return 0;
}

