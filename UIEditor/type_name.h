#ifndef __TYPE_NAME_H_INCLUDED__
#define __TYPE_NAME_H_INCLUDED__

#include "Serialization.h"


template<class T> 
static const char* type_name () {
    return T::type_name ();
}

template<> static const char* type_name<float> ()        { return "float"; }
template<> static const char* type_name<double> ()       { return "double"; }
template<> static const char* type_name<int> ()          { return "int"; }
template<> static const char* type_name<char> ()         { return "char"; }

#endif
