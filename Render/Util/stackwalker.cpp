#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <tchar.h>

#include "Stackwalker.h"

// #############################################################################################
#ifdef _IMAGEHLP_
#error "'imagehlp.h' should only included here, not before this point! Otherwise there are some problems!"
#endif
#pragma pack( push, before_imagehlp, 8 )
#include <imagehlp.h>
#pragma pack( pop, before_imagehlp )
#if API_VERSION_NUMBER < 7  // ImageHelp-Version is older.... so define it by mayself
// The following definition is only available with VC++ 6.0 or higher, so include it here
extern "C" {
//
// source file line data structure
//
typedef struct _IMAGEHLP_LINE
{
    DWORD                       SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE)
    DWORD                       Key;                    // internal
    DWORD                       LineNumber;             // line number in file
    PCHAR                       FileName;               // full filename
    DWORD                       Address;                // first instruction of line
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;
#define SYMOPT_LOAD_LINES        0x00000010
}  // extern "C"
#endif



// ##########################################################################################
const int TTBUFLEN=8096;

#define gle (GetLastError())
#define lenof(a) (sizeof(a) / sizeof((a)[0]))
#define IMGSYMLEN ( sizeof IMAGEHLP_SYMBOL )

typedef BOOL (__stdcall *tSymCleanup)( IN HANDLE hProcess );
typedef PVOID (__stdcall *tSymFunctionTableAccess)( HANDLE hProcess, DWORD AddrBase );
typedef BOOL (__stdcall *tSymGetLineFromAddr)( IN HANDLE hProcess, IN DWORD dwAddr,
  OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE Line );
typedef DWORD (__stdcall *tSymGetModuleBase)( IN HANDLE hProcess, IN DWORD dwAddr );
typedef BOOL (__stdcall *tSymGetModuleInfo)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PIMAGEHLP_MODULE ModuleInfo );
typedef DWORD (__stdcall *tSymGetOptions)( VOID );
typedef BOOL (__stdcall *tSymGetSymFromAddr)( IN HANDLE hProcess, IN DWORD dwAddr,
  OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_SYMBOL Symbol );
typedef BOOL (__stdcall *tSymInitialize)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
typedef DWORD (__stdcall *tSymLoadModule)( IN HANDLE hProcess, IN HANDLE hFile,
  IN PSTR ImageName, IN PSTR ModuleName, IN DWORD BaseOfDll, IN DWORD SizeOfDll );
typedef DWORD (__stdcall *tSymSetOptions)( IN DWORD SymOptions );
typedef BOOL (__stdcall *tStackWalk)( DWORD MachineType, HANDLE hProcess,
  HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress );
typedef DWORD (__stdcall WINAPI *tUnDecorateSymbolName)( PCSTR DecoratedName, PSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags );

struct sStack
{
	tSymCleanup pSymCleanup;
	tSymFunctionTableAccess pSymFunctionTableAccess;
	tSymGetLineFromAddr pSymGetLineFromAddr;
	tSymGetModuleBase pSymGetModuleBase;
	tSymGetModuleInfo pSymGetModuleInfo;
	tSymGetOptions pSymGetOptions;
	tSymGetSymFromAddr pSymGetSymFromAddr;
	tSymInitialize pSymInitialize;
	tSymLoadModule pSymLoadModule;
	tSymSetOptions pSymSetOptions;
	tStackWalk pStackWalk;
	tUnDecorateSymbolName pUnDecorateSymbolName;

	bool IsLoadAll(){ return pSymCleanup && pSymFunctionTableAccess && pSymGetModuleBase && pSymGetModuleInfo &&
	  pSymGetOptions && pSymGetSymFromAddr && pSymInitialize && pSymSetOptions &&
	  pStackWalk && pUnDecorateSymbolName && pSymLoadModule;};

	HINSTANCE g_hImagehlpDll;
//	CRITICAL_SECTION g_csFileOpenClose;
};

static bool LoadModuleInfo(sStack* stack);

// **************************************** ToolHelp32 ************************
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
typedef struct tagMODULEENTRY32
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;
#pragma pack( pop )


void LoadModule(LPCSTR image_name,LPCSTR module_name,DWORD base_addres,DWORD size,
				sStack* stack)
{
    stack->pSymLoadModule( GetCurrentProcess(), 0, (LPSTR)image_name, (LPSTR)module_name,
		base_addres, size );
}


static bool GetModuleListTH32(DWORD pid,sStack* stack)
{
  // CreateToolhelp32Snapshot()
  typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
  // Module32First()
  typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
  // Module32Next()
  typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

  // try both dlls...
  const TCHAR *dllname[] = { _T("kernel32.dll"), _T("tlhelp32.dll") };
  HINSTANCE hToolhelp;
  tCT32S pCT32S;
  tM32F pM32F;
  tM32N pM32N;

  HANDLE hSnap;
  MODULEENTRY32 me;
  me.dwSize = sizeof(me);
  bool keepGoing;
  int i;

  for (i = 0; i<lenof(dllname); i++ )
  {
    hToolhelp = LoadLibrary( dllname[i] );
    if (hToolhelp == NULL)
      continue;
    pCT32S = (tCT32S) GetProcAddress(hToolhelp, "CreateToolhelp32Snapshot");
    pM32F = (tM32F) GetProcAddress(hToolhelp, "Module32First");
    pM32N = (tM32N) GetProcAddress(hToolhelp, "Module32Next");
    if ( pCT32S != 0 && pM32F != 0 && pM32N != 0 )
      break; // found the functions!
    FreeLibrary(hToolhelp);
    hToolhelp = NULL;
  }

  if (hToolhelp == NULL)
    return false;

  hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
  if (hSnap == (HANDLE) -1)
    return false;

  bool ok=false;
  keepGoing = !!pM32F( hSnap, &me );
  while (keepGoing)
  {
	ok=true;
	LoadModule(me.szExePath,me.szModule,(DWORD) me.modBaseAddr,me.modBaseSize,stack);

    keepGoing = !!pM32N( hSnap, &me );
  }

  CloseHandle(hSnap);
  FreeLibrary(hToolhelp);

  return ok;
}  // GetModuleListTH32


// **************************************** PSAPI ************************
typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

static bool GetModuleListPSAPI(DWORD pid,sStack* stack, HANDLE hProcess)
{
  // EnumProcessModules()
  typedef BOOL (__stdcall *tEPM)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded );
  // GetModuleFileNameEx()
  typedef DWORD (__stdcall *tGMFNE)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
  // GetModuleBaseName()
  typedef DWORD (__stdcall *tGMBN)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
  // GetModuleInformation()
  typedef BOOL (__stdcall *tGMI)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize );

  HINSTANCE hPsapi;
  tEPM pEPM;
  tGMFNE pGMFNE;
  tGMBN pGMBN;
  tGMI pGMI;

  DWORD i;
  DWORD cbNeeded;
  MODULEINFO mi;
  HMODULE *hMods = 0;

  hPsapi = LoadLibrary( _T("psapi.dll") );
  if ( hPsapi == 0 )
    return false;

  pEPM = (tEPM) GetProcAddress( hPsapi, "EnumProcessModules" );
  pGMFNE = (tGMFNE) GetProcAddress( hPsapi, "GetModuleFileNameExA" );
  pGMBN = (tGMFNE) GetProcAddress( hPsapi, "GetModuleBaseNameA" );
  pGMI = (tGMI) GetProcAddress( hPsapi, "GetModuleInformation" );
  if ( pEPM == 0 || pGMFNE == 0 || pGMBN == 0 || pGMI == 0 )
  {
    // we couldn´t find all functions
    FreeLibrary( hPsapi );
    return false;
  }

  hMods = (HMODULE*) heap_alloc(sizeof(HMODULE) * (TTBUFLEN / sizeof HMODULE));

  char *image_name =(char*) heap_alloc(sizeof(char) * TTBUFLEN);
  char *module_name =(char*) heap_alloc(sizeof(char) * TTBUFLEN);
  bool ok=false;

  if ( ! pEPM( hProcess, hMods, TTBUFLEN, &cbNeeded ) )
  {
    goto cleanup;
  }

  if ( cbNeeded > TTBUFLEN )
  {
    goto cleanup;
  }

  for ( i = 0; i < cbNeeded / sizeof hMods[0]; i++ )
  {
	  ok=true;
    // base address, size
    pGMI(hProcess, hMods[i], &mi, sizeof mi );
    // image file name
    image_name[0] = 0;
    pGMFNE(hProcess, hMods[i], image_name, TTBUFLEN );
    // module name
    module_name[0] = 0;
    pGMBN(hProcess, hMods[i], module_name, TTBUFLEN );


	LoadModule(image_name,module_name,(DWORD) mi.lpBaseOfDll,mi.SizeOfImage,
				stack);

  }

cleanup:
  if (hPsapi)
    FreeLibrary(hPsapi);
  heap_free(image_name);
  heap_free(module_name);
  heap_free(hMods);

  return ok;
}


static bool GetModuleList(DWORD pid,sStack* stack, HANDLE hProcess)
{
  // first try toolhelp32
  if (GetModuleListTH32(pid, stack) )
    return true;
  // then try psapi
  return GetModuleListPSAPI(pid, stack, hProcess);
}  // GetModuleList

static sStack* InitStackWalk()
{
	static bool bFirstTime = true;
	static bool g_bInitialized = false;
	static sStack stack;
	if(g_bInitialized)
		return &stack;

	if(!bFirstTime)
		return NULL;
	bFirstTime=false;

	stack.g_hImagehlpDll = LoadLibrary( _T("dbghelp.dll") );
	if ( stack.g_hImagehlpDll == NULL )
	{
		g_bInitialized = false;
		return NULL;
	}

	HINSTANCE dll=stack.g_hImagehlpDll;
	stack.pSymCleanup = (tSymCleanup) GetProcAddress(dll, "SymCleanup" );
	stack.pSymFunctionTableAccess = (tSymFunctionTableAccess) GetProcAddress(dll, "SymFunctionTableAccess" );
	stack.pSymGetLineFromAddr = (tSymGetLineFromAddr) GetProcAddress(dll, "SymGetLineFromAddr" );
	stack.pSymGetModuleBase = (tSymGetModuleBase) GetProcAddress(dll, "SymGetModuleBase" );
	stack.pSymGetModuleInfo = (tSymGetModuleInfo) GetProcAddress(dll, "SymGetModuleInfo" );
	stack.pSymGetOptions = (tSymGetOptions) GetProcAddress(dll, "SymGetOptions" );
	stack.pSymGetSymFromAddr = (tSymGetSymFromAddr) GetProcAddress(dll, "SymGetSymFromAddr" );
	stack.pSymInitialize = (tSymInitialize) GetProcAddress(dll, "SymInitialize" );
	stack.pSymSetOptions = (tSymSetOptions) GetProcAddress(dll, "SymSetOptions" );
	stack.pStackWalk = (tStackWalk) GetProcAddress(dll, "StackWalk" );
	stack.pUnDecorateSymbolName = (tUnDecorateSymbolName) GetProcAddress(dll, "UnDecorateSymbolName" );
	stack.pSymLoadModule = (tSymLoadModule) GetProcAddress(dll, "SymLoadModule" );

	if (!stack.IsLoadAll())
	{
		FreeLibrary( stack.g_hImagehlpDll );
		g_bInitialized = FALSE;
		return NULL;
	}

	g_bInitialized = true;
//	InitializeCriticalSection(&stack.g_csFileOpenClose);

	if(LoadModuleInfo(&stack))
		return &stack;

	//DeInitAllocCheck();
    stack.pSymCleanup( GetCurrentProcess() );
    FreeLibrary( stack.g_hImagehlpDll );
//    DeleteCriticalSection(&stack.g_csFileOpenClose);
	g_bInitialized=false;
	return NULL;
}

static bool LoadModuleInfo(sStack* stack)
{
	HANDLE hProcess = GetCurrentProcess();
    char *tt = (char*) heap_alloc(sizeof(char) * TTBUFLEN);
    if (!tt) 
		return false;

	*tt=0;
	char* cur=tt;
	char* ttmax=tt+TTBUFLEN;

    // current directory
    if ( GetCurrentDirectoryA( TTBUFLEN, cur ) )
	{
		cur+=strlen(cur);
		*cur++=';';
		*cur=0;
	}
    // dir with executable
    if ( GetModuleFileNameA( 0, cur, ttmax-cur) )
    {
		char* p;
      for (p = cur + strlen(cur) - 1; p >= cur; -- p )
      {
        // locate the rightmost path separator
        if ( *p == '\\' || *p == '/' || *p == ':' )
          break;
      }
      // if we found one, p is pointing at it; if not, tt only contains
      // an exe name (no path), and p points before its first byte
      if ( p != cur ) // path sep found?
      {
        if ( *p == ':' ) // we leave colons in place
          ++ p;
		*p++=';';
        *p = '\0'; // eliminate the exe name and last path sep
		cur=p;
      }
    }
    // environment variable _NT_SYMBOL_PATH
    if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", cur, ttmax-cur) )
	{
		cur+=strlen(cur);
		*cur++=';';
		*cur=0;
	}

    // environment variable _NT_ALTERNATE_SYMBOL_PATH
    if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", cur, ttmax-cur ) )
	{
		cur+=strlen(cur);
		*cur++=';';
		*cur=0;
	}
    // environment variable SYSTEMROOT
    if ( GetEnvironmentVariableA( "SYSTEMROOT", cur, ttmax-cur ) )
	{
		cur+=strlen(cur);
		*cur++=';';
		*cur=0;
	}

    if ( cur[-1]==';' ) // if we added anything, we have a trailing semicolon
      cur[-1]=0;

    // init symbol handler stuff (SymInitialize())
    if ( ! stack->pSymInitialize( hProcess, tt, false ) )
    {
      if (tt) heap_free( tt );
      return false;
    }

    // SymGetOptions()
	DWORD symOptions; // symbol handler settings
    symOptions = stack->pSymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES;
    symOptions &= ~SYMOPT_UNDNAME;
    symOptions &= ~SYMOPT_DEFERRED_LOADS;
    stack->pSymSetOptions( symOptions ); // SymSetOptions()

    // Enumerate modules and tell imagehlp.dll about them.
    // On NT, this is not necessary, but it won't hurt.
	GetModuleList(GetCurrentProcessId(),stack, hProcess);

    if (tt) 
      heap_free( tt );
	return true;
}

bool DebugInfoByAddr(DWORD addres,LPSTR file_name,int& line,LPSTR func_name,int buf_max)
{

	HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
	sStack* stack=InitStackWalk();
	if ( !stack )
		return false;

	IMAGEHLP_LINE Line;
	memset( &Line, 0, sizeof Line );
	Line.SizeOfStruct = sizeof Line;

	DWORD offsetFromSymbol=0;
	DWORD offsetFromLine=0;
    func_name[0] = 0;

	BYTE pSymBuffer[ IMGSYMLEN + MAXNAMELEN];
	IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL*)&pSymBuffer;
	memset( pSym, 0, IMGSYMLEN + MAXNAMELEN );
	pSym->SizeOfStruct = IMGSYMLEN;
	pSym->MaxNameLength = MAXNAMELEN;

	if ( ! stack->pSymGetSymFromAddr( hProcess, addres, &offsetFromSymbol, pSym ) )
	{
		return false;
	}
	else
	{
		stack->pUnDecorateSymbolName( pSym->Name, func_name, buf_max, UNDNAME_NAME_ONLY );
	}

	offsetFromLine = 0;
	if ( stack->pSymGetLineFromAddr != NULL )
	{ // yes, we have SymGetLineFromAddr()
		if ( ! stack->pSymGetLineFromAddr( hProcess, addres, &offsetFromLine, &Line ) )
		{
			  return false;
		}
	}

	strncpy(file_name,Line.FileName,buf_max);
	line=Line.LineNumber;
	return true;
}

#ifdef _COMPATIBILITY_PROCESS_
bool ProcessStack(tProcessStack fProcessStack,DWORD param)
{
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread;
	hThread=GetCurrentThread();
	CONTEXT c;
	memset( &c, 0, sizeof c );
	c.ContextFlags = CONTEXT_FULL;

	if ( GetThreadContext( hThread, &c )  == 0)
	  return false;

	sStack* stack=InitStackWalk();
	if ( !stack )
		return false;

//	EnterCriticalSection(&stack->g_csFileOpenClose);

	STACKFRAME s;
	memset( &s, 0, sizeof s );

	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;

	for (int frameNum = 0; ; ++ frameNum )
	{
		if ( ! stack->pStackWalk( IMAGE_FILE_MACHINE_I386, hProcess, hThread, &s, NULL,
				NULL, stack->pSymFunctionTableAccess, stack->pSymGetModuleBase, NULL ) )
		  break;

		if (s.AddrPC.Offset == 0)
			return false;
		if(frameNum==0)
			continue;
		if(s.AddrReturn.Offset==0)
			break;

		fProcessStack(s.AddrPC.Offset,param);

	}

	SetLastError( 0 );
//	LeaveCriticalSection(&stack->g_csFileOpenClose);
	return true;
}
#else 

#ifdef _DEBUG
#define _BY_EBP_
#endif

#pragma optimize("y",off)

#ifdef _BY_EBP_
static DWORD FindTop()
{
	DWORD top;
	DWORD cur;
	__asm
	{
		mov [cur],ebp
	}
	top=cur;
	while(cur)
	{
		top=cur;
		if(!((DWORD*)cur)[1])
			break;
		cur=*(DWORD*)cur;
	}
	return top;
}

bool ProcessStack(tProcessStack fProcessStack,DWORD param)
{
	DWORD cur;
	__asm
	{
		mov [cur],ebp
	}

	static DWORD top=0,bottom=0;
	if(!top)
	{
		top=FindTop();
		const maxstack=2000000;
		if(top>maxstack)
			bottom=top-maxstack;
		else
			bottom=65536;
	}

	DWORD cur_bottom=cur-1;

//	while(cur<top && cur>=bottom )
	while(cur<top && cur>cur_bottom )
	{
		DWORD addr=((DWORD*)cur)[1];
		if(!addr)
			break;
		if(addr>0x70000000)
			break;
		fProcessStack( addr,param);
		cur_bottom=cur;
		cur=*(DWORD*)cur;
	}
	return true;
}
#else
DWORD stack_top=0,stack_bottom=0;
DWORD prog_begin=0x00400000+5,prog_end=0x00400000+5000000;
bool ProcessStack(tProcessStack fProcessStack,DWORD param)
{//By esp
	if(!stack_top)
	{
		__asm
		{
			mov [stack_top],esp
		}

		stack_top=(stack_top+0x10000)&0xFFFF0000;

		const int maxstack=2000000;
		if(stack_top>maxstack)
			stack_bottom=stack_top-maxstack;
		else
			stack_bottom=1024;
	}

	DWORD cur;
	__asm
	{
		mov [cur],esp
	}

	while(cur<stack_top && cur>=stack_bottom )
	{
		cur+=4;
		DWORD addr=*(DWORD*)cur;
		if(addr<prog_begin || addr>prog_end)
			continue;

		BYTE call=((BYTE*)addr)[-5];
		if(call!=0xE8)
			continue;
		DWORD call_addr=((DWORD*)addr)[-1];
		call_addr+=addr;
		if(call_addr<prog_begin || call_addr>prog_end)
			continue;

		fProcessStack( addr-5,param);
	}
	return true;
}
#endif _BY_EBP_

#endif _COMPATIBILITY_PROCESS_

void * __cdecl heap_alloc(size_t size)
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

void __cdecl heap_free(void * p)
{
    HeapFree( GetProcessHeap(), 0, p );
}
