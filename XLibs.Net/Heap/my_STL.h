////////////////////////////////////////////////////////////////////////////////
//
// Additional include for making STL more convenient
//
// Author: Alexandre Kotliar, K-D Lab
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __STL_ADDITION_H__
#define __STL_ADDITION_H__

//Это очень тонкий макрос, он должен быть либо везде, либо нигде.
//#define _STLP_NO_IOSTREAMS 1

// For disabling some warning 
#pragma warning( disable : 4786 4284 4800)
//#pragma warning( disable : 4018)
#pragma warning( disable : 4244 4018)
#pragma warning( disable : 4554 4996)

#ifdef __ICL
#pragma warning( disable : 880 1125)
#endif

#ifndef FOR_EACH
#define FOR_EACH(list, iterator) \
  for(iterator = (list).begin(); iterator != (list).end(); ++iterator)
#endif

#endif // __STL_ADDITION_H__