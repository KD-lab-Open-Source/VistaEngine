// Portable CPUID stub for the cross-platform build.
//
// The real CPUID.CPP uses x86 `__asm cpuid` to probe MMX/feature flags — there
// is no arm64 equivalent and the engine only uses this for a startup capability
// log, so we report "no x86 feature bits".
#include "XGLOBAL.H"

unsigned int xt_get_cpuid()
{
	return 0;
}

char* xt_getMMXstatus()
{
	static char status[] = "";
	return status;
}
