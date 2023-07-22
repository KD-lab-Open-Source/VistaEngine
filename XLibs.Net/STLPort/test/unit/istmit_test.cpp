#include <algorithm>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#include <sstream>
#include <functional>
#include <iterator>
#include <vector>
#include <string>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class IStreamIteratorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(IStreamIteratorTest);
  CPPUNIT_TEST(istmit1);
  //CPPUNIT_TEST(copy_n_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  void istmit1();
  void copy_n_test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IStreamIteratorTest);

#if !defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
typedef istream_iterator<char> istream_char_ite;
typedef istream_iterator<int> istream_int_ite;
typedef istream_iterator<string> istream_string_ite;
#else
typedef istream_iterator<char, ptrdiff_t> istream_char_ite;
typedef istream_iterator<int, ptrdiff_t> istream_int_ite;
typedef istream_iterator<string, ptrdiff_t> istream_string_ite;
#endif

//
// tests implementation
//
void IStreamIteratorTest::istmit1()
{
  const char* buff = "MyString";
  istringstream istr(buff);
  
  char buffer[100];
  size_t i = 0;
  istr.unsetf(ios::skipws); // Disable white-space skipping.
  istream_char_ite s(istr), meos;
  while (!(s == meos)  &&
  //*TY 01/10/1999 - added end of stream check 
  // NOTE operator!= should not be used here ifndef _STLP_FUNCTION_TMPL_PARTIAL_ORDER
         (*s != '\n') &&
         (i < sizeof(buffer) / sizeof(buffer[0]))) {  //*TY 07/28/98 - added index check
    buffer[i++] = *s++;
  }
  buffer[i] = '\0'; // Null terminate buffer.

  CPPUNIT_ASSERT(!strcmp(buffer, buff));

  {
    istringstream empty_istr;
    CPPUNIT_ASSERT( istream_char_ite(empty_istr) == istream_char_ite() );
  }
}

void IStreamIteratorTest::copy_n_test()
{
  //This test check that no character is lost while reading the istream
  //through a istream_iterator.
  {
    istringstream istr("aabbcd");
    string chars;
    copy_n(copy_n(istream_char_ite(istr), 2, back_inserter(chars)).first, 
          2, back_inserter(chars));
    CPPUNIT_ASSERT( chars == "aabb" );
    copy_n(istream_char_ite(istr), 2, back_inserter(chars));
    CPPUNIT_ASSERT( chars == "aabbcd" );
  }

  {
    istringstream istr("11 22 AA BB 33 44 CC DD");
    vector<int> ints;
    vector<string> strings;

    copy_n(istream_int_ite(istr), 2, back_inserter(ints));
    CPPUNIT_ASSERT( ints.size() == 2 );
    CPPUNIT_ASSERT( ints[0] == 11 );
    CPPUNIT_ASSERT( ints[1] == 22 );
    ints.clear();
    copy_n(istream_string_ite(istr), 2, back_inserter(strings));
    CPPUNIT_ASSERT( strings.size() == 2 );
    CPPUNIT_ASSERT( strings[0] == "AA" );
    CPPUNIT_ASSERT( strings[1] == "BB" );
    strings.clear();
    copy_n(istream_int_ite(istr), 2, back_inserter(ints));
    CPPUNIT_ASSERT( ints.size() == 2 );
    CPPUNIT_ASSERT( ints[0] == 33 );
    CPPUNIT_ASSERT( ints[1] == 44 );
    copy_n(istream_string_ite(istr), 2, back_inserter(strings));
    CPPUNIT_ASSERT( strings.size() == 2 );
    CPPUNIT_ASSERT( strings[0] == "CC" );
    CPPUNIT_ASSERT( strings[1] == "DD" );
  }

  {
    istringstream is("1 2 3 4 5 6 7 8 9 10");
    vector<int> ints;
    istream_iterator<int> itr(is);
    itr = copy_n(itr, 0, back_inserter(ints)).first;
    CPPUNIT_ASSERT( ints.empty() );
    itr = copy_n(itr, -1, back_inserter(ints)).first;
    CPPUNIT_ASSERT( ints.empty() );
    itr = copy_n(itr, 2, back_inserter(ints)).first;
    CPPUNIT_ASSERT( ints.size() == 2 );
    CPPUNIT_ASSERT( ints[0] == 1 );
    CPPUNIT_ASSERT( ints[1] == 2 );
    itr = copy_n(itr, 2, back_inserter(ints)).first;
    CPPUNIT_ASSERT( ints.size() == 4 );
    CPPUNIT_ASSERT( ints[2] == 3 );
    CPPUNIT_ASSERT( ints[3] == 4 );
    itr = copy_n(itr, 2, back_inserter(ints)).first;
    CPPUNIT_ASSERT( ints.size() == 6 );
    CPPUNIT_ASSERT( ints[4] == 5 );
    CPPUNIT_ASSERT( ints[5] == 6 );
  }
}

#endif
