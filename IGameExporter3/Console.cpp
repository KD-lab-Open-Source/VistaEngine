#include "stdafx.h"
#include <stdio.h>
#include "Console.h"
#include <malloc.h>
//#include "meshres.h"

char* buffer=0;
int cur=0,size=0;

static HWND console_window=0;
static HWND richedit=0;
static RECT richedit_rc;

void ClearConsole()
{
	if(buffer)
		free(buffer);
	buffer=0;
	cur=size=0;
}

static void ScreenToClient(HWND hwnd,RECT* rc)
{
	ScreenToClient(hwnd,(POINT*)rc);
	ScreenToClient(hwnd,((POINT*)rc)+1);
}

static void GetDlgClientRect(HWND hDlg,HWND h,RECT* rc)
{
	GetWindowRect(h,rc);
	ScreenToClient(hDlg,rc);
}

static void OnSize(WPARAM flag,int cx,int cy)
{
	MoveWindow(richedit,0,richedit_rc.top,
		cx,cy-richedit_rc.top,TRUE);
}

static
bool createRichedit(HWND hwnd)
{
	RECT clientRect; 
	GetClientRect(hwnd, &clientRect); 
	richedit = CreateWindowEx(0, RICHEDIT_CLASS, 0,
		WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL |  ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 
		0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,  hwnd, 0, 0, 0);
	SendMessage(richedit, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS);
	DWORD eventMask = SendMessage(richedit, EM_GETEVENTMASK, 0, 0);
//    SendMessage(edit_, WM_SETFONT, (WPARAM)defaultFont_, 0);
	return bool(richedit);
}

static 
LRESULT CALLBACK DialogProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_DESTROY:
		if(richedit)
			DestroyWindow(richedit);
		richedit=0;
		console_window = 0;
		return 0;
		//break;
	case WM_CREATE:
		createRichedit(hwnd);
		SetWindowText(richedit,buffer);
		GetDlgClientRect(hwnd,richedit,&richedit_rc);
		return TRUE;

	case WM_SIZE:
		OnSize(wParam,LOWORD(lParam),HIWORD(lParam));
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowConsole(HWND parent)
{
	if(buffer==0)return;
	HINSTANCE hInstance=GetModuleHandle(0);
	static bool first=true;
	const char* class_name="MessageConsole";
	if(first)
	{
		first=false;
		LoadLibrary("Riched20.dll");

		WNDCLASSEX wcx; 
		wcx.cbSize = sizeof(wcx);
		wcx.style = CS_HREDRAW |CS_VREDRAW;
		wcx.lpfnWndProc = &DialogProc;
		wcx.cbClsExtra = 0;
		wcx.cbWndExtra = 0;
		wcx.hInstance = 0;
		wcx.hIcon = LoadIcon(0,IDI_APPLICATION);
		wcx.hCursor = LoadCursor(0,IDC_ARROW);
		wcx.hbrBackground = (HBRUSH)GetStockObject(COLOR_BTNFACE);
		wcx.lpszMenuName =  "MainMenu";
		wcx.lpszClassName = class_name;
		wcx.hIconSm = 0;
		if(!RegisterClassEx(&wcx))
			xassert(0 && "Unable to register class");

	}

	//if(console_window )
	//{
	//	::SendMessage(console_window, WM_DESTROY,0,0);
	//	//DestroyWindow(console_window );
	//}
	if(!console_window)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		int consoleWidth = screenWidth * 2/ 3;
		int consoleHeight = screenHeight / 3;
		console_window = ::CreateWindowEx(WS_EX_TOPMOST,
			class_name,
			"Messages...",
			WS_OVERLAPPEDWINDOW,
			screenWidth - consoleWidth,
			screenHeight - consoleHeight,
			consoleWidth,
			consoleHeight,
			parent,
			0,
			hInstance,
			0);
		//	DialogBox(hInstance,MAKEINTRESOURCE(IDD_CONSOL),parent,(DLGPROC)DialogProc);
		ShowWindow(console_window,SW_SHOW);
	}else
	{
		SetWindowText(richedit,buffer);
		SetForegroundWindow(console_window);
	}


	ClearConsole();
}


void AddText(LPCTSTR text)
{
	const bufadd=0x1000;
	int len=strlen(text);

	if(buffer==0)
	{
		size=bufadd;
		buffer=(char*)malloc(bufadd);
	}

	if(size<cur+len+1)
	{
		size+=bufadd;
		buffer=(char*)realloc(buffer,size);
	}

	strcpy(buffer+cur,text);
	cur+=len;
}

void Msg(const char* format,...)
{
	va_list args;
	va_start(args,format);
	VMsg(format,args);
}

void VMsg(const char *fmt, va_list marker)
{
	char buffer[256];
	char buffer1[256];
	vsprintf( buffer, fmt, marker);
	
	for(char *p=buffer,*p1=buffer1;*p;p++,p1++)
	{
		if(*p=='\n')
			*p1++='\r';

		*p1=*p;
	}

	*p1=0;

	AddText(buffer1);
}
