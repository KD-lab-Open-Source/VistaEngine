#pragma once
// MSVC <crtdbg.h> stub — debug heap functions not available on non-Windows
#define _ASSERT(e) ((void)(e))
#define _ASSERTE(e) ((void)(e))
#define _CrtSetDbgFlag(f)
#define _CrtDumpMemoryLeaks() 0
