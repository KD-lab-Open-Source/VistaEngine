#include <algorithm>
#include <string>

#include "cppunit/cppunit_proxy.h"

//This is an STLport specific test case:
#if defined (STLPORT)

#  if defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class TypeTraitsTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TypeTraitsTest);
  CPPUNIT_TEST(manips);
  CPPUNIT_TEST(integer);
  CPPUNIT_TEST(rational);
  CPPUNIT_TEST(pointer_type);
#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  CPPUNIT_TEST(reference_type);
#endif
  CPPUNIT_TEST(both_pointer_type);
  CPPUNIT_TEST(ok_to_use_memcpy);
  CPPUNIT_TEST(trivial_destructor);
  CPPUNIT_TEST_SUITE_END();

protected:
  void manips();
  void integer();
  void rational();
  void pointer_type();
#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  void reference_type();
#endif
  void both_pointer_type();
  void ok_to_use_memcpy();
  void trivial_destructor();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TypeTraitsTest);

int type_to_value(__true_type const&) {
  return 1;
}
int type_to_value(__false_type const&) {
  return 0;
}

int* int_pointer;
int const* int_const_pointer;
int volatile* int_volatile_pointer;
int const volatile* int_const_volatile_pointer;
int int_val = 0;
int const int_const_val = 0;
int volatile int_volatile_val = 0;
int & int_ref = int_val;
int const& int_const_ref = int_val;
int const volatile& int_const_volatile_ref = int_val;

//A type that represent any type:
struct any_type
{};

any_type any;
any_type* any_pointer;
any_type const* any_const_pointer;
any_type volatile* any_volatile_pointer;
any_type const volatile* any_const_volatile_pointer;

//A type that represent any pod type
struct any_pod_type
{};

any_pod_type any_pod;
any_pod_type* any_pod_pointer;
any_pod_type const* any_pod_const_pointer;
any_pod_type volatile* any_pod_volatile_pointer;
any_pod_type const volatile* any_pod_const_volatile_pointer;

namespace std {
  _STLP_TEMPLATE_NULL
  struct __type_traits<any_pod_type> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
  };
}

struct base
{};
struct derived : base
{};

//
// tests implementation
//
template <typename _Tp1, typename _Tp2>
int are_same_uncv_types(_Tp1, _Tp2) {
  typedef typename _AreSameUnCVTypes<_Tp1, _Tp2>::_Ret _Ret;
  return type_to_value(_Ret());
}
#if defined(_STLP_USE_PARTIAL_SPEC_WORKAROUND)
template <typename _From, typename _To>
int is_convertible(_From, _To) {
  typedef typename _IsConvertibleType<_From, _To>::_Type _Ret;
  return type_to_value(_Ret());
}
#endif

void TypeTraitsTest::manips()
{
  {
    typedef __bool2type<0>::_Ret _ZeroRet;
    CPPUNIT_ASSERT( type_to_value(_ZeroRet()) == 0 );
    typedef __bool2type<1>::_Ret _OneRet;
    CPPUNIT_ASSERT( type_to_value(_OneRet()) == 1 );
    typedef __bool2type<65456873>::_Ret _AnyRet;
    CPPUNIT_ASSERT( type_to_value(_AnyRet()) == 1 );
  }

  {
    CPPUNIT_ASSERT( __type2bool<__true_type>::_Ret == 1 );
    CPPUNIT_ASSERT( __type2bool<__false_type>::_Ret == 0 );
    CPPUNIT_ASSERT( __type2bool<any_type>::_Ret == 1 );
  }

  {
    typedef _Not<__true_type>::_Ret _NotTrueRet;
    CPPUNIT_ASSERT( type_to_value(_NotTrueRet()) == 0 );
    typedef _Not<__false_type>::_Ret _NotFalseRet;
    CPPUNIT_ASSERT( type_to_value(_NotFalseRet()) == 1 );
  }

  {
    typedef _Land2<__true_type, __true_type>::_Ret _TrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueRet()) == 1 );
    typedef _Land2<__true_type, __false_type>::_Ret _TrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseRet()) == 0 );
    typedef _Land2<__false_type, __true_type>::_Ret _FalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueRet()) == 0 );
    typedef _Land2<__false_type, __false_type>::_Ret _FalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseRet()) == 0 );
  }

  {
    typedef _Land3<__true_type, __true_type, __true_type>::_Ret _TrueTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueTrueRet()) == 1 );
    typedef _Land3<__true_type, __true_type, __false_type>::_Ret _TrueTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueFalseRet()) == 0 );
    typedef _Land3<__true_type, __false_type, __true_type>::_Ret _TrueFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseTrueRet()) == 0 );
    typedef _Land3<__true_type, __false_type, __false_type>::_Ret _TrueFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseFalseRet()) == 0 );
    typedef _Land3<__false_type, __true_type, __true_type>::_Ret _FalseTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueTrueRet()) == 0 );
    typedef _Land3<__false_type, __true_type, __false_type>::_Ret _FalseTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueFalseRet()) == 0 );
    typedef _Land3<__false_type, __false_type, __true_type>::_Ret _FalseFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseTrueRet()) == 0 );
    typedef _Land3<__false_type, __false_type, __false_type>::_Ret _FalseFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseFalseRet()) == 0 );
  }

  {
    typedef _Lor2<__true_type, __true_type>::_Ret _TrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueRet()) == 1 );
    typedef _Lor2<__true_type, __false_type>::_Ret _TrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseRet()) == 1 );
    typedef _Lor2<__false_type, __true_type>::_Ret _FalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueRet()) == 1 );
    typedef _Lor2<__false_type, __false_type>::_Ret _FalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseRet()) == 0 );
  }

  {
    typedef _Lor3<__true_type, __true_type, __true_type>::_Ret _TrueTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueTrueRet()) == 1 );
    typedef _Lor3<__true_type, __true_type, __false_type>::_Ret _TrueTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueFalseRet()) == 1 );
    typedef _Lor3<__true_type, __false_type, __true_type>::_Ret _TrueFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseTrueRet()) == 1 );
    typedef _Lor3<__true_type, __false_type, __false_type>::_Ret _TrueFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseFalseRet()) == 1 );
    typedef _Lor3<__false_type, __true_type, __true_type>::_Ret _FalseTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueTrueRet()) == 1 );
    typedef _Lor3<__false_type, __true_type, __false_type>::_Ret _FalseTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueFalseRet()) == 1 );
    typedef _Lor3<__false_type, __false_type, __true_type>::_Ret _FalseFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseTrueRet()) == 1 );
    typedef _Lor3<__false_type, __false_type, __false_type>::_Ret _FalseFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseFalseRet()) == 0 );
  }

  {
    typedef __select<1, __true_type, __false_type>::_Ret _SelectFirstRet;
    CPPUNIT_ASSERT( type_to_value(_SelectFirstRet()) == 1 );
    typedef __select<0, __true_type, __false_type>::_Ret _SelectSecondRet;
    CPPUNIT_ASSERT( type_to_value(_SelectSecondRet()) == 0 );
  }

  {
    CPPUNIT_ASSERT( are_same_uncv_types(int_val, int_val) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_val, int_const_val) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_val, int_volatile_val) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_const_val, int_val) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_volatile_val, int_val) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_val, int_pointer) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_pointer, int_val) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_pointer, int_pointer) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_pointer, int_const_pointer) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_pointer, int_const_volatile_pointer) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_const_pointer, int_const_pointer) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(int_const_pointer, int_const_volatile_pointer) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(any, any) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(any, any_pointer) == 0 );
    CPPUNIT_ASSERT( are_same_uncv_types(any_pointer, any_pointer) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(any_const_pointer, any_const_pointer) == 1 );
    CPPUNIT_ASSERT( are_same_uncv_types(any_const_volatile_pointer, any_const_volatile_pointer) == 1 );
  }

#if defined(_STLP_USE_PARTIAL_SPEC_WORKAROUND)
  {
    CPPUNIT_ASSERT( is_convertible(any, base()) == 0 );
    CPPUNIT_ASSERT( is_convertible(derived(), base()) == 1 );
  }
#endif
}

template <typename _Type>
int is_integer(_Type) {
  typedef typename _Is_integer<_Type>::_Integral _Ret;
  return type_to_value(_Ret());
}
void TypeTraitsTest::integer()
{
  CPPUNIT_ASSERT( is_integer(bool()) == 1 );
  CPPUNIT_ASSERT( is_integer(char()) == 1 );
  typedef signed char signed_char;
  CPPUNIT_ASSERT( is_integer(signed_char()) == 1 );
  typedef unsigned char unsigned_char;
  CPPUNIT_ASSERT( is_integer(unsigned_char()) == 1 );
#  if defined (_STLP_HAS_WCHAR_T)
  CPPUNIT_ASSERT( is_integer(wchar_t()) == 1 );
#  endif
  CPPUNIT_ASSERT( is_integer(short()) == 1 );
  typedef unsigned short unsigned_short;
  CPPUNIT_ASSERT( is_integer(unsigned_short()) == 1 );
  CPPUNIT_ASSERT( is_integer(int()) == 1 );
  typedef unsigned int unsigned_int;
  CPPUNIT_ASSERT( is_integer(unsigned_int()) == 1 );
  CPPUNIT_ASSERT( is_integer(long()) == 1 );
  typedef unsigned long unsigned_long;
  CPPUNIT_ASSERT( is_integer(unsigned_long()) == 1 );
#  if defined (_STLP_LONG_LONG)
  typedef _STLP_LONG_LONG long_long;
  CPPUNIT_ASSERT( is_integer(long_long()) == 1 );
  typedef unsigned _STLP_LONG_LONG unsigned_long_long;
  CPPUNIT_ASSERT( is_integer(unsigned_long_long()) == 1 );
#endif
  CPPUNIT_ASSERT( is_integer(float()) == 0 );
  CPPUNIT_ASSERT( is_integer(double()) == 0 );
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_ASSERT( is_integer(long_double()) == 0 );
#  endif
  CPPUNIT_ASSERT( is_integer(any) == 0 );
  CPPUNIT_ASSERT( is_integer(any_pointer) == 0 );
}

template <typename _Type>
int is_rational(_Type) {
  typedef typename _Is_rational<_Type>::_Rational _Ret;
  return type_to_value(_Ret());
}
void TypeTraitsTest::rational()
{
  CPPUNIT_ASSERT( is_rational(bool()) == 0 );
  CPPUNIT_ASSERT( is_rational(char()) == 0 );
  typedef signed char signed_char;
  CPPUNIT_ASSERT( is_rational(signed_char()) == 0 );
  typedef unsigned char unsigned_char;
  CPPUNIT_ASSERT( is_rational(unsigned_char()) == 0 );
#  if defined (_STLP_HAS_WCHAR_T)
  CPPUNIT_ASSERT( is_rational(wchar_t()) == 0 );
#  endif
  CPPUNIT_ASSERT( is_rational(short()) == 0 );
  typedef unsigned short unsigned_short;
  CPPUNIT_ASSERT( is_rational(unsigned_short()) == 0 );
  CPPUNIT_ASSERT( is_rational(int()) == 0 );
  typedef unsigned int unsigned_int;
  CPPUNIT_ASSERT( is_rational(unsigned_int()) == 0 );
  CPPUNIT_ASSERT( is_rational(long()) == 0 );
  typedef unsigned long unsigned_long;
  CPPUNIT_ASSERT( is_rational(unsigned_long()) == 0 );
#  if defined (_STLP_LONG_LONG)
  typedef _STLP_LONG_LONG long_long;
  CPPUNIT_ASSERT( is_rational(long_long()) == 0 );
  typedef unsigned _STLP_LONG_LONG unsigned_long_long;
  CPPUNIT_ASSERT( is_rational(unsigned_long_long()) == 0 );
#endif
  CPPUNIT_ASSERT( is_rational(float()) == 1 );
  CPPUNIT_ASSERT( is_rational(double()) == 1 );
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_ASSERT( is_rational(long_double()) == 1 );
#  endif
  CPPUNIT_ASSERT( is_rational(any) == 0 );
  CPPUNIT_ASSERT( is_rational(any_pointer) == 0 );
}

template <typename _Type>
int is_pointer_type(_Type) {
  return type_to_value(_IsPtrType<_Type>::_Ret());
}

void TypeTraitsTest::pointer_type()
{
  CPPUNIT_ASSERT( is_pointer_type(int_val) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(int_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_ref) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_ref) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(any) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(any_pointer) == 1 );
}

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
void TypeTraitsTest::reference_type()
{
  CPPUNIT_ASSERT( _IsRef<int>::_Ret == 0 );
  CPPUNIT_ASSERT( _IsRef<int*>::_Ret == 0 );
  CPPUNIT_ASSERT( _IsRef<int&>::_Ret == 1 );
  CPPUNIT_ASSERT( _IsRef<int const&>::_Ret == 1 );
  CPPUNIT_ASSERT( _IsRef<int const volatile&>::_Ret == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int>::_Ret()) == 0 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int*>::_Ret()) == 0 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int&>::_Ret()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int const&>::_Ret()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int const volatile&>::_Ret()) == 1 );

  CPPUNIT_ASSERT( type_to_value(_OKToSwap<int, int, int&, int&>::_Answer()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsOKToSwap(int_pointer, int_pointer, __true_type(), __true_type())._Answer()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsOKToSwap(int_pointer, int_pointer, __false_type(), __false_type())._Answer()) == 0 );
}
#endif

template <typename _Tp1, typename _Tp2>
int are_both_pointer_type (_Tp1, _Tp2) {
  return type_to_value(_BothPtrType<_Tp1, _Tp2>::_Ret());
}
void TypeTraitsTest::both_pointer_type()
{
  CPPUNIT_ASSERT( are_both_pointer_type(int_val, int_val) == 0 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_pointer, int_pointer) == 1 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_const_pointer, int_const_pointer) == 1 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_volatile_pointer, int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_const_volatile_pointer, int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_ref, int_ref) == 0 );
  CPPUNIT_ASSERT( are_both_pointer_type(int_const_ref, int_const_ref) == 0 );
  CPPUNIT_ASSERT( are_both_pointer_type(any, any) == 0 );
  CPPUNIT_ASSERT( are_both_pointer_type(any_pointer, any_pointer) == 1 );
}

template <typename _Tp1, typename _Tp2>
int is_ok_to_use_memcpy(_Tp1 val1, _Tp2 val2) {
  return type_to_value(_IsOKToMemCpy(val1, val2)._Answer());
}
void TypeTraitsTest::ok_to_use_memcpy()
{
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_pointer, int_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_const_pointer, int_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_pointer, int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_pointer, int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_const_pointer, int_const_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_const_pointer, int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_const_pointer, int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_const_volatile_pointer, int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(int_pointer, any_pointer) == 0 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pointer, int_pointer) == 0 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pointer, any_pointer) == 0 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pointer, any_const_pointer) == 0 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pod_pointer, int_pointer) == 0 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pod_pointer, any_pod_pointer) == 1 );
  CPPUNIT_ASSERT( is_ok_to_use_memcpy(any_pod_pointer, any_pod_const_pointer) == 1 );
}

template <typename _Tp>
int has_trivial_destructor(_Tp val) {
  typedef typename __type_traits<_Tp>::has_trivial_destructor _TrivialDestructor;
  return type_to_value(_TrivialDestructor());
}
void TypeTraitsTest::trivial_destructor()
{
  CPPUNIT_ASSERT( has_trivial_destructor(int_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(int_const_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(any_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(any) == 0 );
  CPPUNIT_ASSERT( has_trivial_destructor(any_pointer) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(any_pod) == 1 );
  CPPUNIT_ASSERT( has_trivial_destructor(string()) == 0 );
}

#endif //STLPORT
