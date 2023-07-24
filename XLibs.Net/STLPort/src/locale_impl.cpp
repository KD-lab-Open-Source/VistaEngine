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
#include "stlport_prefix.h"

#include "locale_impl.h"
#include <locale>
#include <typeinfo>
#include <algorithm>

#include <stdexcept>

#include "c_locale.h"
#include "aligned_buffer.h"

#include "locale_impl.h"
#include <stl/_codecvt.h>
#include <stl/_collate.h>
#include <stl/_ctype.h>
#include <stl/_monetary.h>
#include "message_facets.h"

_STLP_BEGIN_NAMESPACE

static const string _Nameless("*");

inline bool is_C_locale_name (const char* name) {
  return ((name[0] == 'C') && (name[1] == 0));
}

_Locale_impl * _STLP_CALL _copy_Locale_impl(_Locale_impl *loc)
{
  _STLP_ASSERT( loc != 0 );
  loc->_M_incr();
  _Locale_impl *loc_new = new _Locale_impl(*loc);
  loc->_M_decr();
  return loc_new;
}

locale::facet * _STLP_CALL _get_facet(locale::facet *f)
{
  if (f != 0)
    f->_M_incr();
  return f;
}

void _STLP_CALL _release_facet(locale::facet *&f)
{
  if ((f != 0) && (f->_M_decr() == 0)) {
    if (f->_M_delete)
      delete f;
    f = 0;
  }
}

size_t locale::id::_S_max = 39;

static void _Stl_loc_assign_ids();

static _Stl_aligned_buffer<_Locale_impl::Init> __Loc_init_buf;

_Locale_impl::Init::Init() {
  if (_M_count()._M_incr() == 1) {
    _Locale_impl::_S_initialize();
  }
}

_Locale_impl::Init::~Init() {
  if (_M_count()._M_decr() == 0) {
    _Locale_impl::_S_uninitialize();
  }
}

_Refcount_Base& _Locale_impl::Init::_M_count() const {
  static _Refcount_Base _S_count(0);
  return _S_count;
}

_Locale_impl::_Locale_impl(const char* s)
  : _Refcount_Base(0), name(s), facets_vec() {
  facets_vec.reserve( locale::id::_S_max );
  new (&__Loc_init_buf) Init();
}

_Locale_impl::_Locale_impl( _Locale_impl const& locimpl )
  : _Refcount_Base(0), name(locimpl.name), facets_vec() {
  for_each( locimpl.facets_vec.begin(), locimpl.facets_vec.end(), _get_facet);
  facets_vec = locimpl.facets_vec;
  new (&__Loc_init_buf) Init();
}

_Locale_impl::_Locale_impl( size_t n, const char* s)
  : _Refcount_Base(0), name(s), facets_vec(n, 0) {
  new (&__Loc_init_buf) Init();
}

_Locale_impl::~_Locale_impl() {
  (&__Loc_init_buf)->~Init();
  for_each( facets_vec.begin(), facets_vec.end(), _release_facet);
}

// Initialization of the locale system.  This must be called before
// any locales are constructed.  (Meaning that it must be called when
// the I/O library itself is initialized.)
void _STLP_CALL _Locale_impl::_S_initialize() {
  _Stl_loc_assign_ids();
  make_classic_locale();
}

// Release of the classic locale ressources. Has to be called after the last
// locale destruction and not only after the classic locale destruction as
// the facets can be shared between different facets.
void _STLP_CALL _Locale_impl::_S_uninitialize() {
  //Not necessary anymore as classic facets are now 'normal' dynamically allocated
  //facets with a reference counter telling to _release_facet when the facet can be
  //deleted.
  //free_classic_locale();
}

// _Locale_impl non-inline member functions.
void _STLP_CALL _Locale_impl::_M_throw_bad_cast() {
  _STLP_THROW(bad_cast());  
}

void _Locale_impl::insert( _Locale_impl *from, const locale::id& n ) {
  size_t index = n._M_index;
  if (index > 0 && index < from->size()) {
    this->insert( from->facets_vec[index], index);
  }
}

locale::facet* _Locale_impl::insert(locale::facet *f, size_t index) {
  if (f == 0 || index == 0)
    return 0;

  if (index >= facets_vec.size()) {
    facets_vec.resize(index + 1);
  }
  _release_facet(facets_vec[index]);
  facets_vec[index] = _get_facet(f);

  return f;
}

//
// <locale> content which is dependent on the name
//

template <class Facet>
inline locale::facet* _Locale_insert(_Locale_impl *__that, Facet* f) {
  return __that->insert(f, Facet::id._M_index);
}

/*
 * Six functions, one for each category.  Each of them takes a
 * _Locale* and a name, constructs that appropriate category
 * facets by name, and inserts them into the locale.
 */
void _Locale_impl::insert_ctype_facets(const char* pname) {
  char buf[_Locale_MAX_SIMPLE_NAME];
  _Locale_impl* i2 = locale::classic()._M_impl;

  if (pname == 0 || pname[0] == 0)
    pname = _Locale_ctype_default(buf);

  if (pname == 0 || pname[0] == 0 || is_C_locale_name(pname)) {
    this->insert(i2, ctype<char>::id);
#ifndef _STLP_NO_MBSTATE_T
    this->insert(i2, codecvt<char, char, mbstate_t>::id);
#endif
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, ctype<wchar_t>::id);
#  ifndef _STLP_NO_MBSTATE_T
    this->insert(i2, codecvt<wchar_t, char, mbstate_t>::id);
#  endif
#endif
  } else {
    ctype<char>*    ct                      = 0;
#ifndef _STLP_NO_MBSTATE_T
    codecvt<char, char, mbstate_t>*    cvt  = 0;
#endif
#ifndef _STLP_NO_WCHAR_T
    ctype<wchar_t>* wct                     = 0;
    codecvt<wchar_t, char, mbstate_t>* wcvt = 0;
#endif
    _STLP_TRY {
      ct   = new ctype_byname<char>(pname);
#ifndef _STLP_NO_MBSTATE_T
      cvt  = new codecvt_byname<char, char, mbstate_t>(pname);
#endif
#ifndef _STLP_NO_WCHAR_T
      wct  = new ctype_byname<wchar_t>(pname);
      wcvt = new codecvt_byname<wchar_t, char, mbstate_t>(pname);
#endif
    }

#ifndef _STLP_NO_WCHAR_T
#  ifdef _STLP_NO_MBSTATE_T
    _STLP_UNWIND(delete ct; delete wct; delete wcvt);
#  else
    _STLP_UNWIND(delete ct; delete wct; delete cvt; delete wcvt);
#  endif
#else
#  ifdef _STLP_NO_MBSTATE_T
    _STLP_UNWIND(delete ct);
#  else
    _STLP_UNWIND(delete ct; delete cvt);
#  endif
#endif
    _Locale_insert(this, ct);
#ifndef _STLP_NO_MBSTATE_T
    _Locale_insert(this, cvt);
#endif
#ifndef _STLP_NO_WCHAR_T
    _Locale_insert(this, wct);
    _Locale_insert(this, wcvt);
#endif
  }
}

void _Locale_impl::insert_numeric_facets(const char* pname) {
  _Locale_impl* i2 = locale::classic()._M_impl;

  numpunct<char>* punct = 0;
  num_get<char, istreambuf_iterator<char, char_traits<char> > > *get = 0;
  num_put<char, ostreambuf_iterator<char, char_traits<char> > > *put = 0;
#ifndef _STLP_NO_WCHAR_T
  numpunct<wchar_t>* wpunct = 0;
  num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wget = 0;
  num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wput = 0;
#endif

  char buf[_Locale_MAX_SIMPLE_NAME];
  if (pname == 0 || pname[0] == 0)
    pname = _Locale_numeric_default(buf);

  if (pname == 0 || pname[0] == 0 || is_C_locale_name(pname)) {
    this->insert(i2, numpunct<char>::id);
    this->insert(i2,
                 num_put<char, ostreambuf_iterator<char, char_traits<char> >  >::id);
    this->insert(i2,
                 num_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, numpunct<wchar_t>::id);
    this->insert(i2,
                 num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> >  >::id);
    this->insert(i2,
                 num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif
  }
  else {
    _STLP_TRY {
      punct  = new numpunct_byname<char>(pname);
      get    = new num_get<char, istreambuf_iterator<char, char_traits<char> > >;
      put    = new num_put<char, ostreambuf_iterator<char, char_traits<char> > >;
#ifndef _STLP_NO_WCHAR_T
      wpunct = new numpunct_byname<wchar_t>(pname);
      wget   = new num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
      wput   = new num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete punct; delete wpunct; delete get; delete wget; delete put; delete wput);
#else
    _STLP_UNWIND(delete punct; delete get;delete put);
#endif

  _Locale_insert(this,punct);
  _Locale_insert(this,get);
  _Locale_insert(this,put);

#ifndef _STLP_NO_WCHAR_T
  _Locale_insert(this,wpunct);
  _Locale_insert(this,wget);
  _Locale_insert(this,wput);
#endif
  }
}

void _Locale_impl::insert_time_facets(const char* pname) {
  _Locale_impl* i2 = locale::classic()._M_impl;
  time_get<char, istreambuf_iterator<char, char_traits<char> > > *get = 0;
  time_put<char, ostreambuf_iterator<char, char_traits<char> > > *put = 0;
#ifndef _STLP_NO_WCHAR_T
  time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wget = 0;
  time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wput = 0;
#endif

  char buf[_Locale_MAX_SIMPLE_NAME];
  if (pname == 0 || pname[0] == 0)
    pname = _Locale_time_default(buf);

  if (pname == 0 || pname[0] == 0 || is_C_locale_name(pname)) {

    this->insert(i2,
                 time_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    this->insert(i2,
                 time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2,
                 time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    this->insert(i2,
                 time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif
  } else {
    _STLP_TRY {
      get  = new time_get_byname<char, istreambuf_iterator<char, char_traits<char> > >(pname);
      put  = new time_put_byname<char, ostreambuf_iterator<char, char_traits<char> > >(pname);
#ifndef _STLP_NO_WCHAR_T
      wget = new time_get_byname<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(pname);
      wput = new time_put_byname<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(pname);
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete get; delete wget; delete put; delete wput);
#else
    _STLP_UNWIND(delete get; delete put);
#endif
    _Locale_insert(this,get);
    _Locale_insert(this,put);
#ifndef _STLP_NO_WCHAR_T
    _Locale_insert(this,wget);
    _Locale_insert(this,wput);
#endif
  }
}

void _Locale_impl::insert_collate_facets(const char* nam) {
  _Locale_impl* i2 = locale::classic()._M_impl;

  collate<char> *col = 0;
#ifndef _STLP_NO_WCHAR_T
  collate<wchar_t> *wcol = 0;
#endif

  char buf[_Locale_MAX_SIMPLE_NAME];
  if (nam == 0 || nam[0] == 0)
    nam = _Locale_collate_default(buf);

  if (nam == 0 || nam[0] == 0 || is_C_locale_name(nam)) {
    this->insert(i2, collate<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, collate<wchar_t>::id);
#endif
  }
  else {
    _STLP_TRY {
      col   = new collate_byname<char>(nam);
#ifndef _STLP_NO_WCHAR_T
      wcol  = new collate_byname<wchar_t>(nam);
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete col; delete wcol);
#else
    _STLP_UNWIND(delete col);
#endif
    _Locale_insert(this,col);
#ifndef _STLP_NO_WCHAR_T
    _Locale_insert(this,wcol);
#endif
  }
}

void _Locale_impl::insert_monetary_facets(const char* pname) {
  _Locale_impl* i2 = locale::classic()._M_impl;

  moneypunct<char, false> *punct = 0;
  moneypunct<char, true> *ipunct = 0;
  money_get<char, istreambuf_iterator<char, char_traits<char> > > *get = 0;
  money_put<char, ostreambuf_iterator<char, char_traits<char> > > *put = 0;

#ifndef _STLP_NO_WCHAR_T
  moneypunct<wchar_t, false>* wpunct = 0;
  moneypunct<wchar_t, true>* wipunct = 0;
  money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wget = 0;
  money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > *wput = 0;
#endif

  char buf[_Locale_MAX_SIMPLE_NAME];
  if (pname == 0 || pname[0] == 0)
    pname = _Locale_monetary_default(buf);

  if (pname == 0 || pname[0] == 0 || is_C_locale_name(pname)) {
    this->insert(i2, moneypunct<char, false>::id);
    this->insert(i2, moneypunct<char, true>::id);
    this->insert(i2, money_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    this->insert(i2, money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, moneypunct<wchar_t, false>::id);
    this->insert(i2, moneypunct<wchar_t, true>::id);
    this->insert(i2, money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    this->insert(i2, money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif
  }
  else {
    _STLP_TRY {
      punct   = new moneypunct_byname<char, false>(pname);
      ipunct  = new moneypunct_byname<char, true>(pname);
      get     = new money_get<char, istreambuf_iterator<char, char_traits<char> > >;
      put     = new money_put<char, ostreambuf_iterator<char, char_traits<char> > >;
#ifndef _STLP_NO_WCHAR_T
      wpunct  = new moneypunct_byname<wchar_t, false>(pname);
      wipunct = new moneypunct_byname<wchar_t, true>(pname);
      wget    = new money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
      wput    = new money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete punct; delete ipunct; delete wpunct; delete wipunct; delete get; delete wget; delete put; delete wput);
#else
    _STLP_UNWIND(delete punct; delete ipunct; delete get; delete put);
#endif
    _Locale_insert(this,punct);
    _Locale_insert(this,ipunct);
    _Locale_insert(this,get);
    _Locale_insert(this,put);
#ifndef _STLP_NO_WCHAR_T
    _Locale_insert(this,wget);
    _Locale_insert(this,wpunct);
    _Locale_insert(this,wipunct);
    _Locale_insert(this,wput);
#endif
  }
}

void _Locale_impl::insert_messages_facets(const char* pname) {
  _Locale_impl* i2 = locale::classic()._M_impl;
  messages<char> *msg = 0;
#ifndef _STLP_NO_WCHAR_T
  messages<wchar_t> *wmsg = 0;
#endif

  char buf[_Locale_MAX_SIMPLE_NAME];
  if (pname == 0 || pname[0] == 0)
    pname = _Locale_messages_default(buf);

  if (pname == 0 || pname[0] == 0 || is_C_locale_name(pname)) {
    this->insert(i2, messages<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, messages<wchar_t>::id);
#endif
  }
  else {
    _STLP_TRY {
      msg  = new messages_byname<char>(pname);
#ifndef _STLP_NO_WCHAR_T
      wmsg = new messages_byname<wchar_t>(pname);
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete msg; delete wmsg);
#else
    _STLP_UNWIND(delete msg);
#endif
    _Locale_insert(this,msg);
#ifndef _STLP_NO_WCHAR_T
    _Locale_insert(this,wmsg);
#endif
  }
}

static void _Stl_loc_assign_ids() {
  // This assigns ids to every facet that is a member of a category,
  // and also to money_get/put, num_get/put, and time_get/put
  // instantiated using ordinary pointers as the input/output
  // iterators.  (The default is [io]streambuf_iterator.)

  money_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index          = 8;
  money_get<char, const char*>::id._M_index                                             = 9;
  money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index          = 10;
  money_put<char, char*>::id._M_index                                                   = 11;

  num_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index            = 12;
  num_get<char, const char*>::id._M_index                                               = 13;
  num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index            = 14;
  num_put<char, char*>::id._M_index                                                     = 15;
  time_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index           = 16;
  time_get<char, const char*>::id._M_index                                              = 17;
  time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index           = 18;
  time_put<char, char*>::id._M_index                                                    = 19;

#ifndef _STLP_NO_WCHAR_T
  money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index = 27;
  money_get<wchar_t, const wchar_t*>::id._M_index                                       = 28;
  money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index = 29;
  money_put<wchar_t, wchar_t*>::id._M_index                                             = 30;

  num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index   = 31;
  num_get<wchar_t, const wchar_t*>::id._M_index                                         = 32;
  num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > ::id._M_index  = 33;
  num_put<wchar_t, wchar_t*>::id._M_index                                               = 34;
  time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index  = 35;
  time_get<wchar_t, const wchar_t*>::id._M_index                                        = 36;
  time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index  = 37;
  time_put<wchar_t, wchar_t*>::id._M_index                                              = 38;
#endif
  //  locale::id::_S_max                               = 39;
}

// To access those static instance use the getter below, they guaranty
// a correct initialization.
static locale *_Stl_classic_locale = 0;
static locale *_Stl_global_locale = 0;

locale* _Stl_get_classic_locale() {
  static _Locale_impl::Init init;
  return _Stl_classic_locale;
}

locale* _Stl_get_global_locale() {
  static _Locale_impl::Init init;
  return _Stl_global_locale;
}

#if defined (_STLP_MSVC) || defined (__ICL) || defined (__ISCPP__)
/*
 * The following static variable needs to be initialized before STLport
 * users static variable in order for him to be able to use Standard
 * streams in its variable initialization.
 * This variable is here because MSVC do not allow to change the initialization
 * segment in a given translation unit, iostream.cpp already contains an 
 * initialization segment specification.
 */
#  pragma warning (disable : 4073)
#  pragma init_seg(lib)
#endif

static ios_base::Init _IosInit;

void _Locale_impl::make_classic_locale() {
  // This funcion will be called once: during build classic _Locale_impl

  // The classic locale contains every facet that belongs to a category.
  static _Stl_aligned_buffer<_Locale_impl> _Locale_classic_impl_buf;
  _Locale_impl *classic = new(&_Locale_classic_impl_buf) _Locale_impl("C");

  /*
  static _Messages _Null_messages;

  static _Stl_aligned_buffer<collate<char> > _S_collate_char_buf;
  static _Stl_aligned_buffer<ctype<char> > _S_ctype_char_buf;
#  ifndef _STLP_NO_MBSTATE_T
  static _Stl_aligned_buffer<codecvt<char, char, mbstate_t> > _S_codecvt_char_buf;
#  endif
  static _Stl_aligned_buffer<moneypunct<char, true> > _S_moneypunct_true_char_buf;
  static _Stl_aligned_buffer<moneypunct<char, false> > _S_moneypunct_false_char_buf;
  static _Stl_aligned_buffer<numpunct<char> > _S_numpunct_char_buf;
  static _Stl_aligned_buffer<messages<char> > _S_messages_char_buf;

  static _Stl_aligned_buffer<money_get<char, istreambuf_iterator<char, char_traits<char> > > > _S_money_get_char_buf;
  static _Stl_aligned_buffer<money_put<char, ostreambuf_iterator<char, char_traits<char> > > > _S_money_put_char_buf;
  static _Stl_aligned_buffer<num_get<char, istreambuf_iterator<char, char_traits<char> > > > _S_num_get_char_buf;
  static _Stl_aligned_buffer<num_put<char, ostreambuf_iterator<char, char_traits<char> > > > _S_num_put_char_buf;
  static _Stl_aligned_buffer<time_get<char, istreambuf_iterator<char, char_traits<char> > > > _S_time_get_char_buf;
  static _Stl_aligned_buffer<time_put<char, ostreambuf_iterator<char, char_traits<char> > > > _S_time_put_char_buf;

#ifndef _STLP_NO_WCHAR_T
  static _Stl_aligned_buffer<collate<wchar_t> > _S_collate_wchar_buf;
  static _Stl_aligned_buffer<ctype<wchar_t> > _S_ctype_wchar_buf;
#  ifndef _STLP_NO_MBSTATE_T
  static _Stl_aligned_buffer<codecvt<wchar_t, char, mbstate_t> > _S_codecvt_wchar_buf;
#  endif
  static _Stl_aligned_buffer<moneypunct<wchar_t, true> > _S_moneypunct_true_wchar_buf;
  static _Stl_aligned_buffer<moneypunct<wchar_t, false> > _S_moneypunct_false_wchar_buf;
  static _Stl_aligned_buffer<numpunct<wchar_t> > _S_numpunct_wchar_buf;
  static _Stl_aligned_buffer<messages<wchar_t> > _S_messages_wchar_buf;

  static _Stl_aligned_buffer<money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_money_get_wchar_buf;
  static _Stl_aligned_buffer<money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_money_put_wchar_buf;
  static _Stl_aligned_buffer<num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_num_get_wchar_buf;
  static _Stl_aligned_buffer<num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_num_put_wchar_buf;
  static _Stl_aligned_buffer<time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_time_get_wchar_buf;
  static _Stl_aligned_buffer<time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > > _S_time_put_wchar_buf;
#endif
  */

  locale::facet* classic_facets[] = {
    0,
    new /*(&_S_collate_char_buf)*/ collate<char>(),
    new /*(&_S_ctype_char_buf)*/ ctype<char>(0, false),
#ifndef _STLP_NO_MBSTATE_T
    new /*(&_S_codecvt_char_buf)*/ codecvt<char, char, mbstate_t>(),
#else
    0, 
#endif
    new /*(&_S_moneypunct_true_char_buf)*/ moneypunct<char, true>(),
    new /*(&_S_moneypunct_false_char_buf)*/ moneypunct<char, false>(),
    new /*(&_S_numpunct_char_buf)*/ numpunct<char>(),
    new /*(&_S_messages_char_buf)*/ messages<char>(new _Messages()/*&_Null_messages*/),
    new /*(&_S_money_get_char_buf)*/ money_get<char, istreambuf_iterator<char, char_traits<char> > >(),
    0,
    new /*(&_S_money_put_char_buf)*/ money_put<char, ostreambuf_iterator<char, char_traits<char> > >(),
    0,
    new /*(&_S_num_get_char_buf)*/ num_get<char, istreambuf_iterator<char, char_traits<char> > >(),
    0,
    new /*(&_S_num_put_char_buf)*/ num_put<char, ostreambuf_iterator<char, char_traits<char> > >(),
    0,
    new /*(&_S_time_get_char_buf)*/ time_get<char, istreambuf_iterator<char, char_traits<char> > >(),
    0,
    new /*(&_S_time_put_char_buf)*/ time_put<char, ostreambuf_iterator<char, char_traits<char> > >(),
    0,
#ifndef _STLP_NO_WCHAR_T
    new /*(&_S_collate_wchar_buf)*/ collate<wchar_t>(),
    new /*(&_S_ctype_wchar_buf)*/ ctype<wchar_t>(),

#  ifndef _STLP_NO_MBSTATE_T
    new /*(&_S_codecvt_wchar_buf)*/ codecvt<wchar_t, char, mbstate_t>(),
#  else
    0,
#  endif
    new /*(&_S_moneypunct_true_wchar_buf)*/ moneypunct<wchar_t, true>(),
    new /*(&_S_moneypunct_false_wchar_buf)*/ moneypunct<wchar_t, false>(),
    new /*(&_S_numpunct_wchar_buf)*/ numpunct<wchar_t>(),
    new /*(&_S_messages_wchar_buf)*/ messages<wchar_t>(new _Messages()/*&_Null_messages*/),

    new /*(&_S_money_get_wchar_buf)*/ money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,
    new /*(&_S_money_put_wchar_buf)*/ money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,

    new /*(&_S_num_get_wchar_buf)*/ num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,
    new /*(&_S_num_put_wchar_buf)*/ num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,
    new /*(&_S_time_get_wchar_buf)*/ time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,
    new /*(&_S_time_put_wchar_buf)*/ time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(),
    0,
#endif
    0
  };

  const size_t nb_classic_facets = sizeof(classic_facets) / sizeof(locale::facet *);

  //As construction of a locale from a _Locale_impl do not automatically increment
  //facets counter we have to deal with it ourself:
  for_each(&classic_facets[0], &classic_facets[0] + nb_classic_facets, _get_facet);

  classic->facets_vec.reserve(nb_classic_facets);
  classic->facets_vec.assign(&classic_facets[0], &classic_facets[0] + nb_classic_facets);

  static locale _Locale_classic(classic);
  _Stl_classic_locale = &_Locale_classic;

  static locale _Locale_global(_copy_Locale_impl(classic));
  _Stl_global_locale = &_Locale_global;
}

/*
void _Locale_impl::free_classic_locale() {
  _Locale_impl *classic = _Stl_classic_locale->_M_impl;

  classic->facets_vec[collate<char>::id._M_index]->~facet();
  classic->facets_vec[ctype<char>::id._M_index]->~facet();
#  ifndef _STLP_NO_MBSTATE_T
  classic->facets_vec[codecvt<char, char, mbstate_t>::id._M_index]->~facet();
#  endif
  classic->facets_vec[moneypunct<char, true>::id._M_index]->~facet();
  classic->facets_vec[moneypunct<char, false>::id._M_index]->~facet();
  classic->facets_vec[numpunct<char>::id._M_index]->~facet();
  classic->facets_vec[messages<char>::id._M_index]->~facet();

  classic->facets_vec[money_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();
  classic->facets_vec[money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();
  classic->facets_vec[num_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();
  classic->facets_vec[num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();
  classic->facets_vec[time_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();
  classic->facets_vec[time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index]->~facet();

#ifndef _STLP_NO_WCHAR_T
  classic->facets_vec[collate<wchar_t>::id._M_index]->~facet();
  classic->facets_vec[ctype<wchar_t>::id._M_index]->~facet();
#  ifndef _STLP_NO_MBSTATE_T
  classic->facets_vec[codecvt<wchar_t, char, mbstate_t>::id._M_index]->~facet();
#  endif
  classic->facets_vec[moneypunct<wchar_t, true>::id._M_index]->~facet();
  classic->facets_vec[moneypunct<wchar_t, false>::id._M_index]->~facet();
  classic->facets_vec[numpunct<wchar_t>::id._M_index]->~facet();
  classic->facets_vec[messages<wchar_t>::id._M_index]->~facet();

  classic->facets_vec[money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
  classic->facets_vec[money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
  classic->facets_vec[num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
  classic->facets_vec[num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
  classic->facets_vec[time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
  classic->facets_vec[time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index]->~facet();
#endif
}
*/

// Declarations of (non-template) facets' static data members
// size_t locale::id::_S_max = 39; // made before

_STLP_STATIC_MEMBER_DECLSPEC locale::id collate<char>::id = { 1 };
_STLP_STATIC_MEMBER_DECLSPEC locale::id ctype<char>::id = { 2 };

#ifndef _STLP_NO_MBSTATE_T
_STLP_STATIC_MEMBER_DECLSPEC locale::id codecvt<char, char, mbstate_t>::id = { 3 };
#  ifndef _STLP_NO_WCHAR_T
_STLP_STATIC_MEMBER_DECLSPEC locale::id codecvt<wchar_t, char, mbstate_t>::id = { 22 };
#  endif
#endif

_STLP_STATIC_MEMBER_DECLSPEC locale::id moneypunct<char, true>::id = { 4 };
_STLP_STATIC_MEMBER_DECLSPEC locale::id moneypunct<char, false>::id = { 5 };
_STLP_STATIC_MEMBER_DECLSPEC locale::id numpunct<char>::id = { 6 } ;
_STLP_STATIC_MEMBER_DECLSPEC locale::id messages<char>::id = { 7 };

#ifndef _STLP_NO_WCHAR_T
_STLP_STATIC_MEMBER_DECLSPEC locale::id collate<wchar_t>::id = { 20 };
_STLP_STATIC_MEMBER_DECLSPEC locale::id ctype<wchar_t>::id = { 21 };

_STLP_STATIC_MEMBER_DECLSPEC locale::id moneypunct<wchar_t, true>::id = { 23 } ;
_STLP_STATIC_MEMBER_DECLSPEC locale::id moneypunct<wchar_t, false>::id = { 24 } ;

_STLP_STATIC_MEMBER_DECLSPEC locale::id numpunct<wchar_t>::id = { 25 };
_STLP_STATIC_MEMBER_DECLSPEC locale::id messages<wchar_t>::id = { 26 };
#endif

_STLP_DECLSPEC _Locale_impl* _STLP_CALL _get_Locale_impl(_Locale_impl *loc)
{
  _STLP_ASSERT( loc != 0 );
  loc->_M_incr();
  return loc;
}

void _STLP_CALL _release_Locale_impl(_Locale_impl *& loc)
{
  _STLP_ASSERT( loc != 0 );
  if (loc->_M_decr() == 0) {
    if (*loc != *_Stl_classic_locale)
      delete loc;
    else
      loc->~_Locale_impl();
    loc = 0;
  }
}

_STLP_DECLSPEC _Locale_impl* _STLP_CALL _copy_Nameless_Locale_impl(_Locale_impl *loc)
{
  _STLP_ASSERT( loc != 0 );
  loc->_M_incr();
  _Locale_impl *loc_new = new _Locale_impl(*loc);
  loc->_M_decr();
  loc_new->name = _Nameless;
  return loc_new;
}

_STLP_END_NAMESPACE


// locale use many static functions/pointers from this file:
// to avoid making ones extern, simple #include implementation of locale

#include "locale.cpp"

