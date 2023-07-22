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
#ifndef MESSAGE_FACETS_H
#define MESSAGE_FACETS_H

#include <string>
#include <stl/_messages_facets.h>
#include <stl/_ctype.h>
// #include <istream>
#include <typeinfo>
#include <hash_map>
// #include <map>
#include "c_locale.h"

_STLP_BEGIN_NAMESPACE

// Forward declaration of an opaque type.
struct _Catalog_locale_map;

_Locale_messages* __acquire_messages(const char* name); 
void __release_messages(_Locale_messages* cat);

// Class _Catalog_locale_map.  The reason for this is that, internally,
// a message string is always a char*.  We need a ctype facet to convert
// a string to and from wchar_t, and the user is permitted to provide such
// a facet when calling open().

struct _Catalog_locale_map {
  _Catalog_locale_map() : M(0) {}
  ~_Catalog_locale_map() { if (M) delete M; }

  void insert(nl_catd_type key, const locale& L);
  locale lookup(nl_catd_type key) const;
  void erase(nl_catd_type key);

  typedef hash_map<nl_catd_type, locale, hash<nl_catd_type>, equal_to<nl_catd_type> > map_type;
  map_type *M;

private:                        // Invalidate copy constructor and assignment
  _Catalog_locale_map(const _Catalog_locale_map&);
  void operator=(const _Catalog_locale_map&);
};

/*
 * In glibc nl_catd type is void *, but messages_base::catalog is defined as int
 * by ISO/IEC 14882; The int may be too short to store pointer on 64-bit platforms;
 * Another problem, is that do_open() may return negative value to indicate that no
 * catalog open---this case can't be represented with pointers.
 * The class _Catalog_nl_catd_map intended to make relation between
 * messages_base::catalog and nl_catd handler.
 *
 */

#if defined (_STLP_REAL_LOCALE_IMPLEMENTED) && (defined (_STLP_USE_GLIBC) && !defined (__CYGWIN__))
#  define _STLP_USE_NL_CATD_MAPPING
#endif

class _Catalog_nl_catd_map {
public:
  _Catalog_nl_catd_map()
  {}
  ~_Catalog_nl_catd_map()
  {}

  typedef hash_map<messages_base::catalog, nl_catd_type, hash<messages_base::catalog>, equal_to<messages_base::catalog> > map_type;
  typedef hash_map<nl_catd_type, messages_base::catalog, hash<nl_catd_type>, equal_to<nl_catd_type> > rmap_type;
  // typedef map<messages_base::catalog,nl_catd_type> map_type;
  // typedef map<nl_catd_type,messages_base::catalog> rmap_type;

  messages_base::catalog insert(nl_catd_type cat)
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  { return (messages_base::catalog)cat; }
#else
  ;
#endif

  void erase(messages_base::catalog cat)
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  {}
#else
  ;
#endif

  nl_catd_type operator [] ( messages_base::catalog cat ) const
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  { return cat; }
#else
  { return cat < 0 ? 0 : M[cat]; }
#endif

private:
  _Catalog_nl_catd_map(const _Catalog_nl_catd_map&)
  {}
  _Catalog_nl_catd_map& operator =(const _Catalog_nl_catd_map&)
  { return *this; }

#if defined (_STLP_USE_NL_CATD_MAPPING)
  mutable map_type M;
  mutable rmap_type Mr;
  static int _count;
#endif
};

class _Messages {
public:
  typedef messages_base::catalog catalog;

  _Messages();

  virtual catalog do_open(const string& __fn, const locale& __loc) const;
  virtual string do_get(catalog __c, int __set, int __msgid,
                        const string& __dfault) const;
# ifndef _STLP_NO_WCHAR_T
  virtual wstring do_get(catalog __c, int __set, int __msgid,
                         const wstring& __dfault) const;
# endif
  virtual void do_close(catalog __c) const;
  virtual ~_Messages();
  bool _M_delete;
};

class _Messages_impl : public _Messages {
public:

  _Messages_impl(bool);

  _Messages_impl(bool, _Locale_messages*);

  catalog do_open(const string& __fn, const locale& __loc) const;
  string do_get(catalog __c, int __set, int __msgid,
                const string& __dfault) const;
# ifndef _STLP_NO_WCHAR_T
  wstring do_get(catalog __c, int __set, int __msgid,
                 const wstring& __dfault) const;
# endif
  void do_close(catalog __c) const;
  
  ~_Messages_impl();

private:
  _Locale_messages* _M_message_obj;
  _Catalog_locale_map* _M_map;
  mutable _Catalog_nl_catd_map _M_cat;
};

_STLP_END_NAMESPACE

#endif

// Local Variables:
// mode:C++
// End:
