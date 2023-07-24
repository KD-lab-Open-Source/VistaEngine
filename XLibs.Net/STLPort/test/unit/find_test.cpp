#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class FindTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(FindTest);
  CPPUNIT_TEST(find0);
  CPPUNIT_TEST(find1);
  CPPUNIT_TEST(findif0);
  CPPUNIT_TEST(findif1);
  CPPUNIT_TEST(find_char);
  CPPUNIT_TEST_SUITE_END();

protected:
  void find0();
  void find1();
  void findif0();
  void findif1();
  void find_char();
  static bool odd(int a_);
  static bool div_3(int a_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(FindTest);

//
// tests implementation
//
void FindTest::find0()
{
  int numbers[10] = { 0, 1, 4, 9, 16, 25, 36, 49, 64 };

  int *location = find((int*)numbers, (int*)numbers + 10, 25);
  
  CPPUNIT_ASSERT((location - numbers)==5);

  int *out_range = find((int*)numbers, (int*)numbers + 10, 128);

  CPPUNIT_ASSERT( out_range == (int *)(numbers + 10) );
}

void FindTest::find1()
{
  int years[] = { 1942, 1952, 1962, 1972, 1982, 1992 };

  const unsigned yearCount = sizeof(years) / sizeof(years[0]);
  int* location = find((int*)years, (int*)years + yearCount, 1972);

  CPPUNIT_ASSERT((location - years)==3);
}

void FindTest::findif0()
{
  int numbers[6] = { 2, 4, 8, 15, 32, 64 };
  int *location = find_if((int*)numbers, (int*)numbers + 6, odd);

  CPPUNIT_ASSERT((location - numbers)==3);

  int numbers_even[6] = { 2, 4, 8, 16, 32, 64 };

  int *out_range = find_if((int*)numbers_even, (int*)numbers_even + 6, odd);

  CPPUNIT_ASSERT( out_range == (int *)(numbers_even + 6) );
}

void FindTest::findif1()
{
  typedef vector <int> IntVec;
  IntVec v(10);
  for(int i = 0; (size_t)i < v.size(); ++i)
    v[i] =(i + 1) *(i + 1);
  IntVec::iterator iter;
  iter = find_if(v.begin(), v.end(), div_3);
  CPPUNIT_ASSERT((iter - v.begin())==2);
}

bool FindTest::odd(int a_)
{
  return (a_ % 2) != 0;
}

bool FindTest::div_3(int a_)
{
  return a_ % 3 ? 0 : 1;
}

void FindTest::find_char()
{
  char str[] = "abcdefghij";
  char *d = find((char*)str, (char*)str + sizeof(str) / sizeof(char), 'd');
  CPPUNIT_ASSERT( *d == 'd' );

  const char *e = find((const char*)str, (const char*)str + sizeof(str) / sizeof(char), 'e');
  CPPUNIT_ASSERT( *e == 'e' );

  char *last = find((char*)str, (char*)str + sizeof(str) / sizeof(char), 'x');
  CPPUNIT_ASSERT( last == (char *)(str + sizeof(str) / sizeof(char)));

  const char *clast = find((const char*)str, (const char*)str + sizeof(str) / sizeof(char), 'x');
  CPPUNIT_ASSERT( clast == (const char *)(str + sizeof(str) / sizeof(char)));
}
