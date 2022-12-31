#pragma once
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <my_stl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

void dprintf(char *format, ...);
#define SIZE(ar) (sizeof(ar)/sizeof(ar[0]))