#ifndef __TEMP_PTR_H_INCLUDED__
#define __TEMP_PTR_H_INCLUDED__

#include <type_traits>

// tempPtr(X(...)) — the address of a temporary, legally.
//
// This codebase passes temporaries to pointer parameters all over the render and UI
// code: cCamera::SetFrustum(&Vect2f(0.5f, 0.5f), &sRectangle4f(...), ...). Old MSVC
// allowed it as an extension and clang still does with a warning, but a conforming
// MSVC rejects it outright (C2102: '&' requires l-value), and there is no switch for
// it. The pointer parameters are genuinely optional — callers pass 0 — so they cannot
// simply become references.
//
// Binding the temporary to a reference parameter is what makes it addressable. Its
// lifetime is unchanged and always was sufficient: a temporary lives to the end of the
// full-expression, which is the call it is being passed to.
template<class T>
inline typename std::remove_reference<T>::type* tempPtr(T&& value)
{
	return &value;
}

#endif
