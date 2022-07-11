#include "StdAfx.h"
#include "Sound.h"
#include "resource.h"
#include "mp+\mppdec.h"

HWND g_hWnd=NULL;

static HINSTANCE hinstance;
long vol=0;//255;

static
BOOL CALLBACK DialogProc(HWND hwnd,UINT msg,
						 WPARAM wParam,LPARAM lParam)
{
	g_hWnd=hwnd;

	switch(msg)
	{
	case WM_SYSCOMMAND:
		if(wParam==SC_CLOSE)
		{
			EndDialog(hwnd,IDCANCEL);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hwnd,IDOK);
			break;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hwnd,IDCANCEL);
			break;
		}

		if (LOWORD(wParam) == IDC_PLAY)
		{
			DDPlaySound("b");
			break;
		}

		if (LOWORD(wParam) == IDC_LOW)
		{
			const step=5;
			if(vol-step>0)
				vol-=step;

			MpegSetVolume(vol);
			break;
		}

		if (LOWORD(wParam) == IDC_HIGH)
		{
/*
			FILE* file=//fopen("C:\\GAMES\\Base\\pak1_large.pk3","r+b");
				fopen("C:\\GAMES\\Base\\pak2.pk3","r+b");
			char x[1024];

			while(fread(x,sizeof(x),1,file))
			{
				int k=0;
			}

			fclose(file);
*/

/*
			for(int i=0;i<1000;i++)
			{
				for(int j=0;j<1000;j++)
				{
					for(int l=0;l<1000;l++)
					{
						int k=0;
					}
				}
			}
*/
			//vol=255;
			//MpegSetVolume(vol);
			MpegStreamVolumeTo(0,2000);
			MpegStreamSetVolume(255);
			MpegStreamOpenToPlay("wav\\loop.mp+",true);
			break;
		}

		if(LOWORD(wParam) == IDC_STOP_RESUME)
		{
			HWND h=GetDlgItem(hwnd,IDC_STOP_RESUME);
			MpegState s=MpegIsPlay();
			if(s==MPEG_PLAY)
			{
				MpegPause();
			}
			if(s==MPEG_PAUSE)
			{
				MpegResume();
			}

			s=MpegIsPlay();
			switch(s)
			{
			case MPEG_STOP:SetWindowText(h,"Stoped");break;
			case MPEG_PLAY:SetWindowText(h,"Played");break;
			case MPEG_PAUSE:SetWindowText(h,"Paused");break;
			}

			break;
		}
		
		break;
	case WM_INITDIALOG:
		{
			InitDirectSound(hwnd);
	
			bool b=MpegOpenToPlay(
				//"wav\\a.mp+",
				"wav\\loop.mp+",
				true);
		}
		return TRUE;
	}
	return FALSE;
}


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow)
{
	hinstance=hInstance;
/*
	if(MpegOpen("wav\\track02.mp+"))
	{
		short* buffer;
		int len;
		int k=0;
		while(MpegGetNextSample(buffer,len))
		{
			k++;
		}

		MpegClose();
	}
*/
	
	DialogBox(hinstance,MAKEINTRESOURCE(IDD_DIALOG_ONE),
		NULL,DialogProc);
	MpegClose();

	ReleaseDirectSound();
	return 0;
}
