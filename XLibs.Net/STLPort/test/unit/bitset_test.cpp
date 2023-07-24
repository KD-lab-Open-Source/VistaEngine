#include <bitset>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BitsetTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BitsetTest);
  CPPUNIT_TEST(bitset1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void bitset1();
  static void disp_bitset(char const* pname, bitset<13U> const& bset);
};

CPPUNIT_TEST_SUITE_REGISTRATION(BitsetTest);

//
// tests implementation
//
void BitsetTest::bitset1()
{
  bitset<13U> b1(0xFFFF);
  bitset<13U> b2(0x1111);
  CPPUNIT_ASSERT(b1.size()==13);
  CPPUNIT_ASSERT(b1==0x1FFF);
  CPPUNIT_ASSERT(b2.size()==13);
  CPPUNIT_ASSERT(b2==0x1111);
  
  b1 = b1^(b2<<2);
  CPPUNIT_ASSERT(b1==0x1BBB);

  CPPUNIT_ASSERT(b1.count()==10);
  CPPUNIT_ASSERT(b2.count()==4);
}
