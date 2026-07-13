// Portable XErrorHandler for the cross-platform build.
//
// The real XERRHAND.CPP is the Win32/x86 crash handler: it installs an
// SetUnhandledExceptionFilter, dumps the x86 CONTEXT registers (Eax/Eip/...) and
// a DbgHelp/BugslayerUtil call stack, and reports via MessageBox — none of which
// is portable. Off-Windows we provide the same fatal-error semantics (print the
// message and abort) without the platform-specific machinery. A richer
// cross-platform crash reporter is a later effort.
#include "XGLOBAL.H"
#include <cstdio>
#include <cstdlib>

#pragma init_seg(lib)
XErrorHandler ErrH;

XErrorHandler::XErrorHandler()
{
	prefix      = "XHANDLER  INFORM";
	postfix     = 0;
	restore_func = 0;
	state       = 0;
	flags       = XERR_ALL;
	flag_errorOrAssertHandling = false;
}

XErrorHandler::~XErrorHandler()
{
}

void XErrorHandler::Abort(const char* message, int code, int val, const char* subj)
{
	flag_errorOrAssertHandling = true;

	if(restore_func){
		restore_func();
		restore_func = 0;
	}

	fprintf(stderr, "%s: %s", prefix ? prefix : "ERROR", message ? message : "");
	if(subj)
		fprintf(stderr, "\n%s", subj);
	if(val && val != -1)
		fprintf(stderr, "\nCODE: 0x%x", (unsigned)val);
	fprintf(stderr, "\n");
	fflush(stderr);

	state = 1;
	exit(code);
}

void XErrorHandler::Exit()
{
	if(restore_func){
		restore_func();
		restore_func = 0;
	}
	exit(0);
}

void XErrorHandler::SetPrefix(const char* s)
{
	prefix = s;
}

void XErrorHandler::SetRestore(void (*rf)())
{
	restore_func = rf;
}

// The same condition xutil.h uses to declare these: where the asserts compile out,
// the names are macros or absent, and defining them here would not compile.
#if (!defined(_FINAL_VERSION_) || defined(_DEBUG)) && !defined(NASSERT)

// What xassert() calls on a failure. The Win32 original (XERRHAND/DiagAssert.cpp)
// raised a MessageBox offering Ignore / Break / Abort, with a DbgHelp stack trace
// read out of the x86 CONTEXT. Here: report it and carry on. Returning 1 is that
// dialog's "Ignore", which latches the assert off, so one failing every frame
// reports once instead of flooding.
static void (*assertRestoreGraphics)() = nullptr;

void SetAssertRestoreGraphicsFunction(void(*func)())
{
	assertRestoreGraphics = func;
}

int DiagAssert(unsigned long, const char* szMsg, const char* szFile, unsigned long dwLine)
{
	if(assertRestoreGraphics)
		assertRestoreGraphics();	// leave fullscreen, or the report is invisible

	fprintf(stderr, "ASSERTION FAILED: %s\n  %s:%lu\n",
			szMsg ? szMsg : "", szFile ? szFile : "", dwLine);
	fflush(stderr);
	return 1;
}

#endif
