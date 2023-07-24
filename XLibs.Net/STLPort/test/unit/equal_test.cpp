#include <vector>
#include <algorithm>
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class EqualTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(EqualTest);
  CPPUNIT_TEST(eqlrnge0);
  CPPUNIT_TEST(eqlrnge1);
  CPPUNIT_TEST(eqlrnge2);
  CPPUNIT_TEST(equal0);
  CPPUNIT_TEST(equal1);
  CPPUNIT_TEST(equal2);
  CPPUNIT_TEST(equalto);
  CPPUNIT_TEST_SUITE_END();

protected:
  void eqlrnge0();
  void eqlrnge1();
  void eqlrnge2();
  void equal0();
  void equal1();
  void equal2();
  void equalto();
  static bool values_squared(int a_, int b_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(EqualTest);

//
// tests implementation
//
void EqualTest::eqlrnge0()
{
  int numbers[10] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3 };
  pair <int*, int*> range = equal_range((int*)numbers, (int*)numbers + 10, 2);
  CPPUNIT_ASSERT((range.first - numbers)==4);
  CPPUNIT_ASSERT((range.second - numbers)==8);
}
void EqualTest::eqlrnge1()
{
  typedef vector <int> IntVec;
  IntVec v(10);
  for (int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i / 3;

  pair<IntVec::iterator, IntVec::iterator> range=equal_range(v.begin(), v.end(), 2);
  CPPUNIT_ASSERT((range.first - v.begin())==6);
  CPPUNIT_ASSERT((range.second - v.begin())==9);

}
void EqualTest::eqlrnge2()
{
  char chars[] = "aabbccddggghhklllmqqqqssyyzz";

  const unsigned count = sizeof(chars) - 1;
  pair<char*, char*>range=equal_range((char*)chars, (char*)chars + count, 'q', less<char>());
  CPPUNIT_ASSERT((range.first - chars)==18);
  CPPUNIT_ASSERT((range.second - chars)==22);
}
void EqualTest::equal0()
{
  int numbers1[5] = { 1, 2, 3, 4, 5 };
  int numbers2[5] = { 1, 2, 4, 8, 16 };
  int numbers3[2] = { 1, 2 };

  CPPUNIT_ASSERT(!equal(numbers1, numbers1 + 5, numbers2));
  CPPUNIT_ASSERT(equal(numbers3, numbers3 + 2, numbers1));
}
void EqualTest::equal1()
{
  vector <int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i;
  vector <int> v2(10);
  CPPUNIT_ASSERT(!equal(v1.begin(), v1.end(), v2.begin()));

  copy(v1.begin(), v1.end(), v2.begin());
  CPPUNIT_ASSERT(equal(v1.begin(), v1.end(), v2.begin()))
}
void EqualTest::equal2()
{
  vector <int> v1(10);
  vector <int> v2(10);
  for (int i = 0; (size_t)i < v1.size(); ++i) {
    v1[i] = i;
    v2[i] = i * i;
  }
  CPPUNIT_ASSERT(equal(v1.begin(), v1.end(), v2.begin(), values_squared));
}
void EqualTest::equalto()
{
  int input1 [4] = { 1, 7, 2, 2 };
  int input2 [4] = { 1, 6, 2, 3 };

  int output [4];
  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, equal_to<int>());
  CPPUNIT_ASSERT(output[0]==1);
  CPPUNIT_ASSERT(output[1]==0);
  CPPUNIT_ASSERT(output[2]==1);
  CPPUNIT_ASSERT(output[3]==0);
}
bool EqualTest::values_squared(int a_, int b_)
{
  return(a_ * a_ == b_);
}
