#include <vector>
#include <algorithm>
#if defined (_STLP_USE_EXCEPTIONS)
# include <stdexcept>
#endif

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class VectorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(VectorTest);
  CPPUNIT_TEST(vec_test_1);
  CPPUNIT_TEST(vec_test_2);
  CPPUNIT_TEST(vec_test_3);
  CPPUNIT_TEST(vec_test_4);
  CPPUNIT_TEST(vec_test_5);
  CPPUNIT_TEST(vec_test_6);
  CPPUNIT_TEST(vec_test_7);
  CPPUNIT_TEST(capacity);
  CPPUNIT_TEST(at);
  CPPUNIT_TEST(pointer);
  CPPUNIT_TEST(auto_ref);
  CPPUNIT_TEST_SUITE_END();

protected:
  void vec_test_1();
  void vec_test_2();
  void vec_test_3();
  void vec_test_4();
  void vec_test_5();
  void vec_test_6();
  void vec_test_7();
  void capacity();
  void at();
  void pointer();
  void auto_ref();
};

CPPUNIT_TEST_SUITE_REGISTRATION(VectorTest);

//
// tests implementation
//
void VectorTest::vec_test_1()
{
  vector<int> v1; // Empty vector of integers.

  CPPUNIT_ASSERT( v1.empty() == true );
  CPPUNIT_ASSERT( v1.size() == 0 );

  // CPPUNIT_ASSERT( v1.max_size() == INT_MAX / sizeof(int) );
  // cout << "max_size = " << v1.max_size() << endl;
  v1.push_back(42); // Add an integer to the vector.

  CPPUNIT_ASSERT( v1.size() == 1 );

  CPPUNIT_ASSERT( v1[0] == 42 );
}

void VectorTest::vec_test_2()
{
  vector<double> v1; // Empty vector of doubles.
  v1.push_back(32.1);
  v1.push_back(40.5);
  vector<double> v2; // Another empty vector of doubles.
  v2.push_back(3.56);

  CPPUNIT_ASSERT( v1.size() == 2 );
  CPPUNIT_ASSERT( v1[0] == 32.1 );
  CPPUNIT_ASSERT( v1[1] == 40.5 );

  CPPUNIT_ASSERT( v2.size() == 1 );
  CPPUNIT_ASSERT( v2[0] == 3.56 );
  v1.swap(v2); // Swap the vector's contents.

  CPPUNIT_ASSERT( v1.size() == 1 );
  CPPUNIT_ASSERT( v1[0] == 3.56 );
 
  CPPUNIT_ASSERT( v2.size() == 2 );
  CPPUNIT_ASSERT( v2[0] == 32.1 );
  CPPUNIT_ASSERT( v2[1] == 40.5 );

  v2 = v1; // Assign one vector to another.

  CPPUNIT_ASSERT( v2.size() == 1 );
  CPPUNIT_ASSERT( v2[0] == 3.56 );
}

void VectorTest::vec_test_3()
{
  typedef  vector<char> vec_type;

  vec_type v1; // Empty vector of characters.
  v1.push_back('h');
  v1.push_back('i');

  CPPUNIT_ASSERT( v1.size() == 2 );
  CPPUNIT_ASSERT( v1[0] == 'h' );
  CPPUNIT_ASSERT( v1[1] == 'i' );

  vec_type v2(v1.begin(), v1.end());
  v2[1] = 'o'; // Replace second character.

  CPPUNIT_ASSERT( v2.size() == 2 );
  CPPUNIT_ASSERT( v2[0] == 'h' );
  CPPUNIT_ASSERT( v2[1] == 'o' );
  
  CPPUNIT_ASSERT( (v1 == v2) == false );

  CPPUNIT_ASSERT( (v1 < v2) == true );
}

void VectorTest::vec_test_4()
{
  vector<int> v(4);

  v[0] = 1;
  v[1] = 4;
  v[2] = 9;
  v[3] = 16;

  CPPUNIT_ASSERT( v.front() == 1 );
  CPPUNIT_ASSERT( v.back() == 16 );

  v.push_back(25);

  CPPUNIT_ASSERT( v.back() == 25 );
  CPPUNIT_ASSERT( v.size() == 5 );

  v.pop_back();

  CPPUNIT_ASSERT( v.back() == 16 );
  CPPUNIT_ASSERT( v.size() == 4 );
}

void VectorTest::vec_test_5()
{
  int array [] = { 1, 4, 9, 16 };

  vector<int> v(array, array + 4);

  CPPUNIT_ASSERT( v.size() == 4 );

  CPPUNIT_ASSERT( v[0] == 1 );
  CPPUNIT_ASSERT( v[1] == 4 );
  CPPUNIT_ASSERT( v[2] == 9 );
  CPPUNIT_ASSERT( v[3] == 16 );
}

void VectorTest::vec_test_6()
{
  int array [] = { 1, 4, 9, 16, 25, 36 };

  vector<int> v(array, array + 6);
  vector<int>::iterator vit;

  CPPUNIT_ASSERT( v.size() == 6 );
  CPPUNIT_ASSERT( v[0] == 1 );
  CPPUNIT_ASSERT( v[1] == 4 );
  CPPUNIT_ASSERT( v[2] == 9 );
  CPPUNIT_ASSERT( v[3] == 16 );
  CPPUNIT_ASSERT( v[4] == 25 );
  CPPUNIT_ASSERT( v[5] == 36 );

  vit = v.erase( v.begin() ); // Erase first element.
  CPPUNIT_ASSERT( *vit == 4 );
  
  CPPUNIT_ASSERT( v.size() == 5 );
  CPPUNIT_ASSERT( v[0] == 4 );
  CPPUNIT_ASSERT( v[1] == 9 );
  CPPUNIT_ASSERT( v[2] == 16 );
  CPPUNIT_ASSERT( v[3] == 25 );
  CPPUNIT_ASSERT( v[4] == 36 );

  vit = v.erase(v.end() - 1); // Erase last element.
  CPPUNIT_ASSERT( vit == v.end() );

  CPPUNIT_ASSERT( v.size() == 4 );
  CPPUNIT_ASSERT( v[0] == 4 );
  CPPUNIT_ASSERT( v[1] == 9 );
  CPPUNIT_ASSERT( v[2] == 16 );
  CPPUNIT_ASSERT( v[3] == 25 );


  v.erase(v.begin() + 1, v.end() - 1); // Erase all but first and last.

  CPPUNIT_ASSERT( v.size() == 2 );
  CPPUNIT_ASSERT( v[0] == 4 );
  CPPUNIT_ASSERT( v[1] == 25 );

}

void VectorTest::vec_test_7()
{
  int array1 [] = { 1, 4, 25 };
  int array2 [] = { 9, 16 };

  vector<int> v(array1, array1 + 3);
  vector<int>::iterator vit;
  vit = v.insert(v.begin(), 0); // Insert before first element.
  CPPUNIT_ASSERT( *vit == 0 );
  
  vit = v.insert(v.end(), 36);  // Insert after last element.
  CPPUNIT_ASSERT( *vit == 36 );

  CPPUNIT_ASSERT( v.size() == 5 );
  CPPUNIT_ASSERT( v[0] == 0 );
  CPPUNIT_ASSERT( v[1] == 1 );
  CPPUNIT_ASSERT( v[2] == 4 );
  CPPUNIT_ASSERT( v[3] == 25 );
  CPPUNIT_ASSERT( v[4] == 36 );

  // Insert contents of array2 before fourth element.
  v.insert(v.begin() + 3, array2, array2 + 2);

  CPPUNIT_ASSERT( v.size() == 7 );

  CPPUNIT_ASSERT( v[0] == 0 );
  CPPUNIT_ASSERT( v[1] == 1 );
  CPPUNIT_ASSERT( v[2] == 4 );
  CPPUNIT_ASSERT( v[3] == 9 );
  CPPUNIT_ASSERT( v[4] == 16 );
  CPPUNIT_ASSERT( v[5] == 25 );
  CPPUNIT_ASSERT( v[6] == 36 );

  v.clear();
  CPPUNIT_ASSERT( v.empty() );

  v.insert(v.begin(), 5, 10);
  CPPUNIT_ASSERT( v.size() == 5 );
  CPPUNIT_ASSERT( v[0] == 10 );
  CPPUNIT_ASSERT( v[1] == 10 );
  CPPUNIT_ASSERT( v[2] == 10 );
  CPPUNIT_ASSERT( v[3] == 10 );
  CPPUNIT_ASSERT( v[4] == 10 );
}

void VectorTest::capacity()
{
  vector<int> v;

  CPPUNIT_ASSERT( v.capacity() == 0 );
  v.push_back(42);
  CPPUNIT_ASSERT( v.capacity() == 1 );
  v.reserve(5000);
  CPPUNIT_ASSERT( v.capacity() == 5000 );
}

void VectorTest::at() {
  vector<int> v;
  vector<int> const& cv = v;
  
  v.push_back(10);
  CPPUNIT_ASSERT( v.at(0) == 10 );
  v.at(0) = 20;
  CPPUNIT_ASSERT( cv.at(0) == 20 );
  
#ifdef _STLP_USE_EXCEPTIONS
  for (;;) {
    try {
      v.at(1) = 20;
      CPPUNIT_ASSERT(false);
    }
    catch (std::out_of_range const&) {
      CPPUNIT_ASSERT(true);
      return;
    }
    catch (...) {
      CPPUNIT_ASSERT(false);
      return;
    }
  }
#endif
}

void VectorTest::pointer()
{
  vector<int *> v1;
  vector<int *> v2 = v1;
  vector<int *> v3;
  
  v3.insert( v3.end(), v1.begin(), v1.end() );
}

void VectorTest::auto_ref()
{
  vector<int> ref;
  for (int i = 0; i < 5; ++i) {
    ref.push_back(i);
  }

  vector<vector<int> > v_v_int(1, ref);
  v_v_int.push_back(v_v_int[0]);
  v_v_int.push_back(ref);
  v_v_int.push_back(v_v_int[0]);
  v_v_int.push_back(v_v_int[0]);
  v_v_int.push_back(ref);

  vector<vector<int> >::iterator vvit(v_v_int.begin()), vvitEnd(v_v_int.end());
  for (; vvit != vvitEnd; ++vvit) {
    CPPUNIT_ASSERT( *vvit == ref );
  }

  /*
   * Forbidden by the Standard:
  v_v_int.insert(v_v_int.end(), v_v_int.begin(), v_v_int.end());
  for (vvit = v_v_int.begin(), vvitEnd = v_v_int.end();
       vvit != vvitEnd; ++vvit) {
    CPPUNIT_ASSERT( *vvit == ref );
  }
   */
}
