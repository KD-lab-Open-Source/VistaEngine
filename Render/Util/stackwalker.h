#pragma once

const int MAXNAMELEN=1024; // max name length for found symbols
typedef void (__stdcall *tProcessStack)(DWORD addres,DWORD param);

bool ProcessStack(tProcessStack process,DWORD param);

bool DebugInfoByAddr(DWORD addres,LPSTR file_name,int& line,LPSTR func_name,int buf_max);

void * __cdecl heap_alloc(size_t size);
void __cdecl heap_free(void * p);

