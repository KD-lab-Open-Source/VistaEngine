#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MaxTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MaxTest);
  CPPUNIT_TEST(max1);
  CPPUNIT_TEST(max2);
  CPPUNIT_TEST(maxelem1);
  CPPUNIT_TEST(maxelem2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void max1();
  void max2();
  void maxelem1();
  void maxelem2();

  static bool str_compare(const char* a_, const char* b_)
  {
    return strcmp(a_, b_) < 0 ? 1 : 0;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MaxTest);

//
// tests implementation
//
void MaxTest::max1()
{
  int r = max(42, 100);
  CPPUNIT_ASSERT( r == 100 );

  r = max(++r, r);
  CPPUNIT_ASSERT( r == 101 );
}
void MaxTest::max2()
{
  char* r = max((char *)"shoe",(char *)"shine", str_compare);
  CPPUNIT_ASSERT(!strcmp(r, "shoe"));
}
void MaxTest::maxelem1()
{
  int numbers[6] = { 4, 10, 56, 11, -42, 19 };

  int* r = max_element((int*)numbers, (int*)numbers + 6);
  CPPUNIT_ASSERT(*r==56);
}
void MaxTest::maxelem2()
{
  char* names[] = { "Brett", "Graham", "Jack", "Mike", "Todd" };

  const unsigned namesCt = sizeof(names)/sizeof(names[0]);
  char** r = max_element((char**)names, (char**)names + namesCt, str_compare);
  CPPUNIT_ASSERT(!strcmp(*r, "Todd"));
}
