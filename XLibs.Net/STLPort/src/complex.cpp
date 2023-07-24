/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999 
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */ 
# include "stlport_prefix.h"
// Complex division and square roots.

#include <complex>
#include <cmath>

_STLP_BEGIN_NAMESPACE

// Absolute value
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC float _STLP_CALL abs(const complex<float>& __z) {
  return ::hypot(__z._M_re, __z._M_im);
}
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC double _STLP_CALL abs(const complex<double>& __z) {
  return ::hypot(__z._M_re, __z._M_im);
}

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC long double _STLP_CALL abs(const complex<long double>& __z) {
  return ::hypot(__z._M_re, __z._M_im);
}
#endif

// Phase

_STLP_TEMPLATE_NULL 
_STLP_DECLSPEC float _STLP_CALL arg(const complex<float>& __z) {
  return ::atan2(__z._M_im, __z._M_re);
}

_STLP_TEMPLATE_NULL 
_STLP_DECLSPEC double _STLP_CALL arg(const complex<double>& __z) {
  return ::atan2(__z._M_im, __z._M_re);
}

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC long double _STLP_CALL arg(const complex<long double>& __z) {
  return ::atan2(__z._M_im, __z._M_re);
}
#endif

// Construct a complex number from polar representation
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC complex<float> _STLP_CALL polar(const float& __rho, const float& __phi) {
  return complex<float>(__rho * ::cos(__phi), __rho * ::sin(__phi));
}
_STLP_TEMPLATE_NULL
_STLP_DECLSPEC complex<double> _STLP_CALL polar(const double& __rho, const double& __phi) {
  return complex<double>(__rho * ::cos(__phi), __rho * ::sin(__phi));
}

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_TEMPLATE_NULL 
_STLP_DECLSPEC complex<long double> _STLP_CALL polar(const long double& __rho, const long double& __phi) {
  return complex<long double>(__rho * ::cos(__phi), __rho * ::sin(__phi));
}
#endif

// Division
template <class _Tp>
void _divT(const _Tp& __z1_r, const _Tp& __z1_i,
           const _Tp& __z2_r, const _Tp& __z2_i,
           _Tp& __res_r, _Tp& __res_i) {
  _Tp __ar = __z2_r >= 0 ? __z2_r : -__z2_r;
  _Tp __ai = __z2_i >= 0 ? __z2_i : -__z2_i;

  if (__ar <= __ai) {
    _Tp __ratio = __z2_r / __z2_i;
    _Tp __denom = __z2_i * (1 + __ratio * __ratio);
    __res_r = (__z1_r * __ratio + __z1_i) / __denom;
    __res_i = (__z1_i * __ratio - __z1_r) / __denom;
  }
  else {
    _Tp __ratio = __z2_i / __z2_r;
    _Tp __denom = __z2_r * (1 + __ratio * __ratio);
    __res_r = (__z1_r + __z1_i * __ratio) / __denom;
    __res_i = (__z1_i - __z1_r * __ratio) / __denom;
  }
}

template <class _Tp>
void _divT(const _Tp& __z1_r,
           const _Tp& __z2_r, const _Tp& __z2_i,
           _Tp& __res_r, _Tp& __res_i) {
  _Tp __ar = __z2_r >= 0 ? __z2_r : -__z2_r;
  _Tp __ai = __z2_i >= 0 ? __z2_i : -__z2_i;

  if (__ar <= __ai) {
    _Tp __ratio = __z2_r / __z2_i;
    _Tp __denom = __z2_i * (1 + __ratio * __ratio);
    __res_r = (__z1_r * __ratio) / __denom;
    __res_i = - __z1_r / __denom;
  }
  else {
    _Tp __ratio = __z2_i / __z2_r;
    _Tp __denom = __z2_r * (1 + __ratio * __ratio);
    __res_r = __z1_r / __denom;
    __res_i = - (__z1_r * __ratio) / __denom;
  }
}

void _STLP_CALL
complex<float>::_div(const float& __z1_r, const float& __z1_i,
                     const float& __z2_r, const float& __z2_i,
                     float& __res_r, float& __res_i) {
  _divT(__z1_r, __z1_i, __z2_r, __z2_i, __res_r, __res_i);
}

void _STLP_CALL
complex<float>::_div(const float& __z1_r,
                     const float& __z2_r, const float& __z2_i,
                     float& __res_r, float& __res_i) {
  _divT(__z1_r, __z2_r, __z2_i, __res_r, __res_i);
}


void  _STLP_CALL
complex<double>::_div(const double& __z1_r, const double& __z1_i,
                      const double& __z2_r, const double& __z2_i,
                      double& __res_r, double& __res_i) {
  _divT(__z1_r, __z1_i, __z2_r, __z2_i, __res_r, __res_i);
}

void _STLP_CALL
complex<double>::_div(const double& __z1_r,
                      const double& __z2_r, const double& __z2_i,
                      double& __res_r, double& __res_i) {
  _divT(__z1_r, __z2_r, __z2_i, __res_r, __res_i);
}

#if !defined (_STLP_NO_LONG_DOUBLE)
void  _STLP_CALL
complex<long double>::_div(const long double& __z1_r, const long double& __z1_i,
                           const long double& __z2_r, const long double& __z2_i,
                           long double& __res_r, long double& __res_i) {
  _divT(__z1_r, __z1_i, __z2_r, __z2_i, __res_r, __res_i);
}


void _STLP_CALL
complex<long double>::_div(const long double& __z1_r,
                           const long double& __z2_r, const long double& __z2_i,
                           long double& __res_r, long double& __res_i) {
  _divT(__z1_r, __z2_r, __z2_i, __res_r, __res_i);
}
#endif

//----------------------------------------------------------------------
// Square root
template <class _Tp>
complex<_Tp> sqrtT(const complex<_Tp>& z) {
  _Tp re = z._M_re;
  _Tp im = z._M_im;
  _Tp mag = ::hypot(re, im);
  complex<_Tp> result;

  if (mag == 0.f) {
    result._M_re = result._M_im = 0.f;
  } else if (re > 0.f) {
    result._M_re = ::sqrt(0.5f * (mag + re));
    result._M_im = im/result._M_re/2.f;
  } else {
    result._M_im = ::sqrt(0.5f * (mag - re));
    if (im < 0.f)
      result._M_im = - result._M_im;
    result._M_re = im/result._M_im/2.f;
  }
  return result;
}

complex<float> _STLP_CALL
sqrt(const complex<float>& z) { return sqrtT(z); }

complex<double>  _STLP_CALL
sqrt(const complex<double>& z) { return sqrtT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
complex<long double> _STLP_CALL
sqrt(const complex<long double>& z) { return sqrtT(z); }
#endif

_STLP_END_NAMESPACE
