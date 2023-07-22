#include <slist>
#include <algorithm>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <sstream>
#endif
#include <iterator>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class SlistTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SlistTest);
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
  CPPUNIT_TEST(slist1);
#endif
  CPPUNIT_TEST(erase);
  CPPUNIT_TEST(insert);
  CPPUNIT_TEST(splice);
  CPPUNIT_TEST_SUITE_END();

protected:
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
  void slist1();
#endif
  void erase();
  void insert();
  void splice();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SlistTest);

//
// tests implementation
//
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
void SlistTest::slist1()
{
/*
original: xlxtss
reversed: sstxlx
removed: sstl
uniqued: stl
sorted: lst
*/
  
  char array [] = { 'x', 'l', 'x', 't', 's', 's' };
  ostringstream os;
  ostream_iterator<char> o(os,"");
  slist<char> str(array+0, array + 6);
  slist<char>::iterator i;
  //Check const_iterator construction from iterator
  slist<char>::const_iterator ci(i);
  slist<char>::const_iterator ci2(ci);
//  cout << "reversed: ";
  str.reverse();
  for(i = str.begin(); i != str.end(); i++)
    os << *i;
  stringbuf* buff=os.rdbuf();
  string result=buff->str();
  CPPUNIT_ASSERT(!strcmp(result.c_str(),"sstxlx"));

  //cout << "removed: ";
  str.remove('x');
  ostringstream os2;
  for(i = str.begin(); i != str.end(); i++)
    os2 << *i;
  buff=os2.rdbuf();
  result=buff->str();
  CPPUNIT_ASSERT(!strcmp(result.c_str(),"sstl"));


  //cout << "uniqued: ";
  str.unique();
  ostringstream os3;
  for(i = str.begin(); i != str.end(); i++)
    os3 << *i;
  buff=os3.rdbuf();
  result=buff->str();
  CPPUNIT_ASSERT(!strcmp(result.c_str(),"stl"));
  
  //cout << "sorted: ";
  str.sort();
  ostringstream os4;
  for(i = str.begin(); i != str.end(); i++)
    os4 << *i;
  buff=os4.rdbuf();
  result=buff->str();
  CPPUNIT_ASSERT(!strcmp(result.c_str(),"lst"));

  //A small compilation time check to be activated from time to time:
#  if 0
  {
    slist<char>::iterator sl_char_ite;
    slist<int>::iterator sl_int_ite;
    CPPUNIT_ASSERT( sl_char_ite != sl_int_ite );
  }
#  endif
}
#endif

void SlistTest::erase()
{
  int array[] = { 0, 1, 2, 3, 4 };
  slist<int> sl(array, array + 5);
  slist<int>::iterator slit;
  
  slit = sl.erase(sl.begin());
  CPPUNIT_ASSERT( *slit == 1);
  
  ++slit++; ++slit;
  slit = sl.erase(sl.begin(), slit);
  CPPUNIT_ASSERT( *slit == 3 );
  
  sl.assign(array, array + 5);
  
  slit = sl.erase_after(sl.begin());
  CPPUNIT_ASSERT( *slit == 2 );
  
  slit = sl.begin(); ++slit; ++slit;
  slit = sl.erase_after(sl.begin(), slit);
  CPPUNIT_ASSERT( *slit == 3 );

  sl.erase_after(sl.before_begin());
  CPPUNIT_ASSERT( sl.front() == 3 );
}

void SlistTest::insert()
{
  int array[] = { 0, 1, 2, 3, 4 };

  //insert
  {
    slist<int> sl;

    sl.insert(sl.begin(), 5);
    CPPUNIT_ASSERT( sl.front() == 5 );
    CPPUNIT_ASSERT( sl.size() == 1 );

    //debug mode check:
    //sl.insert(sl.before_begin(), array, array + 5);

    sl.insert(sl.begin(), array, array + 5);
    CPPUNIT_ASSERT( sl.size() == 6 );
    int i;
    slist<int>::iterator slit(sl.begin());
    for (i = 0; slit != sl.end(); ++slit, ++i) {
      CPPUNIT_ASSERT( *slit == i );
    }
  }

  //insert_after
  {
    slist<int> sl;

    //debug check:
    //sl.insert_after(sl.begin(), 5);

    sl.insert_after(sl.before_begin(), 5);
    CPPUNIT_ASSERT( sl.front() == 5 );
    CPPUNIT_ASSERT( sl.size() == 1 );

    sl.insert_after(sl.before_begin(), array, array + 5);
    CPPUNIT_ASSERT( sl.size() == 6 );
    int i;
    slist<int>::iterator slit(sl.begin());
    for (i = 0; slit != sl.end(); ++slit, ++i) {
      CPPUNIT_ASSERT( *slit == i );
    }
  }
}

void SlistTest::splice()
{
  int array[] = { 0, 1, 2, 3, 4 };

  //splice
  {
    slist<int> sl1(array, array + 5);
    slist<int> sl2(array, array + 5);
    slist<int>::iterator slit;

    //a no op:
    sl1.splice(sl1.begin(), sl1, sl1.begin());
    CPPUNIT_ASSERT( sl1 == sl2 );

    slit = sl1.begin(); ++slit;
    //a no op:
    sl1.splice(slit, sl1, sl1.begin());
    CPPUNIT_ASSERT( sl1 == sl2 );

    sl1.splice(sl1.end(), sl1, sl1.begin());
    slit = sl1.begin();
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *(slit++) == 2 );
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *(slit++) == 4 );
    CPPUNIT_ASSERT( *slit == 0 );
    sl1.splice(sl1.begin(), sl1, slit);
    CPPUNIT_ASSERT( sl1 == sl2 );

    sl1.splice(sl1.begin(), sl2);
    size_t i;
    for (i = 0, slit = sl1.begin(); slit != sl1.end(); ++slit, ++i) {
      if (i == 5) i = 0;
      CPPUNIT_ASSERT( *slit == array[i] );
    }

    slit = sl1.begin();
    advance(slit, 5);
    CPPUNIT_ASSERT( *slit == 0 );
    sl2.splice(sl2.begin(), sl1, sl1.begin(), slit);
    CPPUNIT_ASSERT( sl1 == sl2 );

    slit = sl1.begin(); ++slit;
    sl1.splice(sl1.begin(), sl1, slit, sl1.end());
    slit = sl1.begin();
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *(slit++) == 2 );
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *(slit++) == 4 );
    CPPUNIT_ASSERT( *slit == 0 );

    // a no op
    sl2.splice(sl2.end(), sl2, sl2.begin(), sl2.end());
    for (i = 0, slit = sl2.begin(); slit != sl2.end(); ++slit, ++i) {
      CPPUNIT_ASSERT( i < 5 );
      CPPUNIT_ASSERT( *slit == array[i] );
    }

    slit = sl2.begin();
    advance(slit, 3);
    sl2.splice(sl2.end(), sl2, sl2.begin(), slit);
    slit = sl2.begin();
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *(slit++) == 4 );
    CPPUNIT_ASSERT( *(slit++) == 0 );
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *slit == 2 );
  }

  //splice_after
  {
    slist<int> sl1(array, array + 5);
    slist<int> sl2(array, array + 5);
    slist<int>::iterator slit;

    //a no op:
    sl1.splice_after(sl1.begin(), sl1.begin());
    CPPUNIT_ASSERT( sl1 == sl2 );

    sl1.splice_after(sl1.before_begin(), sl1.begin());
    slit = sl1.begin();
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *(slit++) == 0 );
    CPPUNIT_ASSERT( *(slit++) == 2 );
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *slit == 4 );
    sl1.splice_after(sl1.before_begin(), sl1.begin());
    CPPUNIT_ASSERT( sl1 == sl2 );

    sl1.splice_after(sl1.before_begin(), sl2);
    size_t i;
    for (i = 0, slit = sl1.begin(); slit != sl1.end(); ++slit, ++i) {
      if (i == 5) i = 0;
      CPPUNIT_ASSERT( *slit == array[i] );
    }

    slit = sl1.begin();
    advance(slit, 4);
    CPPUNIT_ASSERT( *slit == 4 );
    sl2.splice_after(sl2.before_begin(), sl1.before_begin(), slit);
    CPPUNIT_ASSERT( sl1 == sl2 );

    sl1.splice_after(sl1.before_begin(), sl1.begin(), sl1.previous(sl1.end()));
    slit = sl1.begin();
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *(slit++) == 2 );
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *(slit++) == 4 );
    CPPUNIT_ASSERT( *slit == 0 );

    // a no op
    sl2.splice_after(sl2.before_begin(), sl2.before_begin(), sl2.previous(sl2.end()));
    for (i = 0, slit = sl2.begin(); slit != sl2.end(); ++slit, ++i) {
      CPPUNIT_ASSERT( i < 5 );
      CPPUNIT_ASSERT( *slit == array[i] );
    }

    slit = sl2.begin();
    advance(slit, 2);
    sl2.splice_after(sl2.previous(sl2.end()), sl2.before_begin(), slit);
    slit = sl2.begin();
    CPPUNIT_ASSERT( *(slit++) == 3 );
    CPPUNIT_ASSERT( *(slit++) == 4 );
    CPPUNIT_ASSERT( *(slit++) == 0 );
    CPPUNIT_ASSERT( *(slit++) == 1 );
    CPPUNIT_ASSERT( *slit == 2 );
  }
}

