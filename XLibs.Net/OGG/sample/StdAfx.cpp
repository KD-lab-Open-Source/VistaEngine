// stdafx.cpp : source file that includes just the standard includes
//	BalmerSample.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

void dprintf(char *format, ...)
{
  va_list args;
  char    buffer[512];

  va_start(args,format);

  strcpy(buffer + vsprintf(buffer,format,args), "\r\n");

  OutputDebugString(buffer);
}
