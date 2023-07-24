#include <exception>
#include <stdexcept>

#include "cppunit/cppunit_proxy.h"

/*
 * This test case purpose is to check that the exception handling
 * functions are correctly imported to the STLport namespace only
 * if they have a right behavior.
 * Otherwise they are not imported to report the problem as a compile
 * time error.
 */

//
// TestCase class
//
#if defined (_STLP_USE_EXCEPTIONS)

#  if !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT) || !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)

class ExceptionTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ExceptionTest);
#    if !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
  CPPUNIT_TEST(unexpected_except);
#    endif
#    if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
  CPPUNIT_TEST(uncaught_except);
#    endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void unexpected_except();
  void uncaught_except();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExceptionTest);

#    if !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
bool g_unexpected_called = false;
void unexpected_hdl() {
  g_unexpected_called = true;
  throw std::bad_exception();
}

struct special_except {};
void throw_func() {
  throw special_except();
}

void throw_except_func() throw(std::exception) {
  throw_func();
}

void ExceptionTest::unexpected_except()
{
  std::unexpected_handler hdl = &unexpected_hdl;
  std::set_unexpected(hdl);

  try {
    throw_except_func();
  }
  catch (std::bad_exception const&) {
    CPPUNIT_ASSERT( true );
  }
  catch (special_except) {
    CPPUNIT_ASSERT( false );
  }
  CPPUNIT_ASSERT( g_unexpected_called );
}
#    endif /* _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT */

#    if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
struct UncaughtClassTest
{
  UncaughtClassTest(int &res) : _res(res)
  {}

  ~UncaughtClassTest() {
    _res = std::uncaught_exception()?1:0;
  }

  int &_res;
};

void ExceptionTest::uncaught_except()
{
  int uncaught_result = -1;
  {
    UncaughtClassTest test_inst(uncaught_result);
    CPPUNIT_ASSERT( uncaught_result == -1 );
  }
  CPPUNIT_ASSERT( uncaught_result == 0 );

  {
    try {
      uncaught_result = -1;
      UncaughtClassTest test_inst(uncaught_result);
      throw "exception";
    }
    catch (...) {
    }
  }
  CPPUNIT_ASSERT( uncaught_result == 1 );
}
#    endif /* _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT */

#  endif /* !_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT || !_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT */

#endif // _STLP_USE_EXCEPTIONS
