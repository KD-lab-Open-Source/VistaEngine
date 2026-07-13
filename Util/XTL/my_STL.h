// FOR_EACH, included by bare name from ~20 places (xglobal.h, XZip, Serialization,
// most StdAfx.h). The original of this header sits in XLibs.Net/Heap next to a pile
// of .exe and .dll files and a second, unrelated Handle.h — putting that directory
// on the include path shadows Util/Serialization/Handle.h, so the macro lives here
// instead, in a directory the build already searches on every platform.
//
// The original also carried a block of #pragma warning(disable: ...) for MSVC 6
// diagnostics; those warnings are either gone or worth hearing, and are not
// reproduced.
#ifndef __STL_ADDITION_H__
#define __STL_ADDITION_H__

#ifndef FOR_EACH
#define FOR_EACH(list, iterator) \
	for((iterator) = (list).begin(); (iterator) != (list).end(); ++(iterator))
#endif

#endif // __STL_ADDITION_H__
