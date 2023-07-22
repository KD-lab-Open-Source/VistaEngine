#include <new>

#include "cppunit/cppunit_proxy.h"

//
// TestCase class
//
class ConfigTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ConfigTest);
  CPPUNIT_TEST(placement_new_bug);
  CPPUNIT_TEST_SUITE_END();

  protected:
    void placement_new_bug();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigTest);


void ConfigTest::placement_new_bug()
{
  int int_val = 1;
  int *pint;
  pint = new(&int_val) int();
  CPPUNIT_ASSERT( pint == &int_val );
#if defined (_STLP_DEF_CONST_PLCT_NEW_BUG)
  CPPUNIT_ASSERT( int_val != 0 );
#else
  CPPUNIT_ASSERT( int_val == 0 );
#endif
}
