#pragma once
// STLPort <stl/_construct.h> compat
#include <new>
#include <iterator>
namespace std {
template<class T1, class T2>
inline void construct(T1* p, const T2& val) { ::new((void*)p) T1(val); }
template<class T>
inline void construct(T* p) { ::new((void*)p) T(); }
template<class T>
inline void destroy(T* p) { p->~T(); }
template<class ForwardIt>
inline void destroy(ForwardIt first, ForwardIt last) { for(; first != last; ++first) destroy(&*first); }
} // namespace std
