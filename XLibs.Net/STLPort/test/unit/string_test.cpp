#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <sstream>
#endif

#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
#  include <stdexcept>
#endif

#if defined (STLPORT) && defined (_STLP_THREADS)
#  ifdef _STLP_PTHREADS
#    include <pthread.h>
#  endif

#  ifdef _STLP_WIN32THREADS
#    include <windows.h>
#  endif
#endif

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class StringTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(StringTest);
  CPPUNIT_TEST(assign);
  CPPUNIT_TEST(erase);
  CPPUNIT_TEST(data);
  CPPUNIT_TEST(c_str);
  CPPUNIT_TEST(insert);
  CPPUNIT_TEST(replace);
  CPPUNIT_TEST(resize);
  CPPUNIT_TEST(short_string);
  CPPUNIT_TEST(find);
#if defined (STLPORT) && defined (_STLP_THREADS)
  CPPUNIT_TEST(mt);
#endif
  CPPUNIT_TEST(short_string_optim_bug);
  CPPUNIT_TEST(compare);
  CPPUNIT_TEST(template_expresion);
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
  CPPUNIT_TEST(io);
#endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void erase();
  void data();
  void c_str();
  void insert();
  void replace();
  void resize();
  void short_string();
  void find();
  void assign();
#if defined (STLPORT) && defined (_STLP_THREADS)
  void mt();
#endif
  void short_string_optim_bug();
  void compare();
  void template_expresion();
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
  void io();
#endif

  static string func(const string& par) {
    string tmp( par );
    return tmp;
  }

#if defined (STLPORT) && defined (_STLP_THREADS)
#  if defined (_STLP_PTHREADS)  || defined (_STLP_UITHREADS)
  static void* f(void*)
#  elif defined (_STLP_WIN32THREADS)
  static DWORD __stdcall f(void*)
#  else
#    error Unknown thread model.
#  endif
  {
    string s( "qyweyuewunfkHBUKGYUGL,wehbYGUW^(@T@H!BALWD:h^&@#*@(#:JKHWJ:CND" );

    for ( int i = 0; i < 2000000; ++i ) {
      string sx = func( s );
    }

    return 0;
  }
#endif

};

CPPUNIT_TEST_SUITE_REGISTRATION(StringTest);

//
// tests implementation
//
#if defined (STLPORT) && defined (_STLP_THREADS)
void StringTest::mt()
{
  const int nth = 2;
#if defined(_STLP_PTHREADS)
  pthread_t t[nth];

  for ( int i = 0; i < nth; ++i ) {
    pthread_create( &t[i], 0, f, 0 );
  }

  for ( int i = 0; i < nth; ++i ) {
    pthread_join( t[i], 0 );
  }
#endif // _STLP_PTHREADS

#if defined (_STLP_WIN32THREADS)
  //DWORD start = GetTickCount();

  HANDLE t[nth];

  int i; // VC6 not support in-loop scope of cycle var
  for ( i = 0; i < nth; ++i ) {
    t[i] = CreateThread(NULL, 0, f, 0, 0, NULL);
  }

#ifdef _STLP_WCE
  // on evc3/evc4 WaitForMultipleObjects() with fWaitAll == TRUE is not supported
  for ( i = 0; i < nth; ++i ) {
    WaitForSingleObject(t[i], INFINITE);
  }
#else
  WaitForMultipleObjects(nth, t, TRUE, INFINITE);
#endif

  /*
  DWORD duration = GetTickCount() - start;
  ostringstream ostr;
  ostr << "Duration: " << duration << endl;
  CPPUNIT_MESSAGE(ostr.str().c_str());
  */
#endif

#if !defined(_STLP_PTHREADS) && !defined(_STLP_WIN32THREADS) && !defined (_STLP_UITHREADS)
  // this test is useless without thread support!
  CPPUNIT_ASSERT(false);
#endif
}
#endif

void StringTest::short_string()
{
  string const ref_short_str1("str1"), ref_short_str2("str2");
  string short_str1(ref_short_str1), short_str2(ref_short_str2);
  string const ref_long_str1("str                                                  1");
  string const ref_long_str2("str                                                  2");
  string long_str1(ref_long_str1), long_str2(ref_long_str2);

  CPPUNIT_ASSERT(short_str1 == ref_short_str1);
  CPPUNIT_ASSERT(long_str1 == ref_long_str1);

  {
    string str1(short_str1);
    str1 = long_str1;
    CPPUNIT_ASSERT(str1 == ref_long_str1);
  }

  {
    string str1(long_str1);
    str1 = short_str1;
    CPPUNIT_ASSERT(str1 == ref_short_str1);
  }

  {
    short_str1.swap(short_str2);
    CPPUNIT_ASSERT((short_str1 == ref_short_str2) && (short_str2 == ref_short_str1));
    short_str1.swap(short_str2);
  }

  {
    long_str1.swap(long_str2);
    CPPUNIT_ASSERT((long_str1 == ref_long_str2) && (long_str2 == ref_long_str1));
    long_str1.swap(long_str2);
  }

  {
    short_str1.swap(long_str1);
    CPPUNIT_ASSERT((short_str1 == ref_long_str1) && (long_str1 == ref_short_str1));
    short_str1.swap(long_str1);
  }

  {
    long_str1.swap(short_str1);
    CPPUNIT_ASSERT((short_str1 == ref_long_str1) && (long_str1 == ref_short_str1));
    long_str1.swap(short_str1);
  }

  {
    //This is to test move constructor
    vector<string> str_vect;
    str_vect.push_back(short_str1);
    str_vect.push_back(long_str1);
    str_vect.push_back(short_str2);
    str_vect.push_back(long_str2);
    CPPUNIT_ASSERT((str_vect[0] == ref_short_str1) &&
                   (str_vect[1] == ref_long_str1) &&
                   (str_vect[2] == ref_short_str2) &&
                   (str_vect[3] == ref_long_str2));
  }
}

void StringTest::erase()
{
  char const* c_str = "Hello, World!";
  std::string str(c_str);
  CPPUNIT_ASSERT( str == c_str );
  
  str.erase(str.begin() + 1, str.end() - 1); // Erase all but first and last.
  
  size_t i;
  for (i = 0; i < str.size(); ++i) {
    switch ( i ) {
      case 0:
        CPPUNIT_ASSERT( str[i] == 'H' );
        break;
      case 1:
        CPPUNIT_ASSERT( str[i] == '!' );
        break;
      default:
        CPPUNIT_ASSERT( false );
        break;
    }
  } 
  
  str.insert(1, (char*)c_str);
  str.erase(str.begin()); // Erase first element.
  str.erase(str.end() - 1); // Erase last element.
  CPPUNIT_ASSERT( str == c_str );
  str.clear(); // Erase all.
  CPPUNIT_ASSERT( str.empty() );

  str = c_str;
  CPPUNIT_ASSERT( str == c_str );

  str.erase(1, str.size() - 1); // Erase all but first and last.
  for (i = 0; i < str.size(); i++) {
    switch ( i ) {
      case 0:
        CPPUNIT_ASSERT( str[i] == 'H' );
        break;
      case 1:
        CPPUNIT_ASSERT( str[i] == '!' );
        break;
      default:
        CPPUNIT_ASSERT( false );
        break;
    }
  }

  str.erase(1);
  CPPUNIT_ASSERT( str == "H" );
}

void StringTest::data()
{
  string xx;

  CPPUNIT_ASSERT( xx.data() != 0 );  // ISO-IEC-14882:1998(E), 21.3.6, paragraph 3
#if 0
  /* This test really not required: in ISO-IEC-14882:1998(E) paragraph 3 stated:
   * '... and can have zero added to it', again: 'CAN', but not 'MUST'.
   * That's why I am comment this test. But I don't remove it due to I had
   * unevident problem with misinterpretation of data() return (i.e. data()
   * and c_str() provide different functionality!) and expect that this is
   * more-or-less common pitfall.
   *    - ptr
   */
  string low( "2004-01-01" );
  // Blocks A and B should follow each other.
  // Block A:
  xx = "123456";
  xx += low;
  if ( strcmp( xx.data(), "1234562004-01-01" ) != 0 ) {
    return -1;
  }
  // End of block A

  // Block B:
  xx = "1234";
  xx += ";";

  if ( strcmp( xx.data(), "1234;" ) != 0 ) {
    return -1;
  }
  // End of block B
#endif
}

void StringTest::c_str()
{
  string low( "2004-01-01" );
  string xx;
  string yy;

  CPPUNIT_ASSERT( *(yy.c_str()) == '\0' ); // ISO-IEC-14882:1998(E), 21.3.6, paragraph 1

  // Blocks A and B should follow each other.
  // Block A:
  xx = "123456";
  xx += low;
  CPPUNIT_ASSERT( strcmp( xx.c_str(), "1234562004-01-01" ) == 0 );
  // End of block A

  // Block B:
  xx = "1234";
  xx += ";";
  CPPUNIT_ASSERT( strcmp( xx.c_str(), "1234;" ) == 0 );
  // End of block B
}

void StringTest::insert()
{
  string strorg = "This is test string for string calls";
  string str;
  /*
   * In case of reallocation there is no auto reference problem
   * so we reserve a big enough string to be sure to test this
   * particular point.
   */
  str.reserve(100);
  str = strorg;

  //test self insertion:
  str.insert(10, str.c_str() + 5, 15);
  CPPUNIT_ASSERT( str == "This is teis test string st string for string calls" );

  str = strorg;
  str.insert(15, str.c_str() + 5, 25);
  CPPUNIT_ASSERT( str == "This is test stis test string for stringring for string calls" );

  str = strorg;
  str.insert(0, str.c_str() + str.size() - 4, 4);
  CPPUNIT_ASSERT( str == "allsThis is test string for string calls" );

  str = strorg;
  str.insert(0, str.c_str() + str.size() / 2 - 1, str.size() / 2 + 1);
  CPPUNIT_ASSERT( str == "ng for string callsThis is test string for string calls" );
  
  str = strorg;
  string::iterator b = str.begin();
  string::const_iterator s = str.begin() + str.size() / 2 - 1;
  string::const_iterator e = str.end();
  str.insert( b, s, e );
  CPPUNIT_ASSERT( str == "ng for string callsThis is test string for string calls" );

  str = strorg;
  str.insert(str.begin(), str.begin() + str.size() / 2 - 1, str.end());
  CPPUNIT_ASSERT( str == "ng for string callsThis is test string for string calls" );

#ifdef _STLP_MEMBER_TEMPLATES
  vector<int> int_vect;
  //Just a compile time test:
  str.insert(str.end(), int_vect.begin(), int_vect.end());
#endif

  string str0;
  str0.insert(str0.begin(), 5, '0');
  CPPUNIT_ASSERT( str0 == "00000" );

  string str1;
  {
    string::size_type pos = 0, nb = 2;
    str1.insert(pos, nb, '1');
  }
  CPPUNIT_ASSERT( str1 == "11" );

  str0.insert(0, str1);
  CPPUNIT_ASSERT( str0 == "1100000" );

  string str2("2345");
  str0.insert(str0.size(), str2, 1, 2);
  CPPUNIT_ASSERT( str0 == "110000034" );

  str1.insert(str1.begin() + 1, 2, '2');
  CPPUNIT_ASSERT( str1 == "1221" );

  str1.insert(2, "333333", 3);
  CPPUNIT_ASSERT( str1 == "1233321" );

  str1.insert(4, "4444");
  CPPUNIT_ASSERT( str1 == "12334444321" );

  str1.insert(str1.begin() + 6, '5');
  CPPUNIT_ASSERT( str1 == "123344544321" );
}

void StringTest::replace()
{
  /*
   * This test case is for the non template basic_string::replace method, 
   * this is why we play with the const iterators and reference to guaranty 
   * that the right method is called.
   */
  const string v( "78" );
  string s( "123456" );
  string const& cs = s;

  string::iterator i = s.begin() + 1;
  s.replace(i, i + 3, v.begin(), v.end());
  CPPUNIT_ASSERT( s == "17856" );
  
  s = "123456";
  i = s.begin() + 1;
  s.replace(i, i + 1, v.begin(), v.end());
  CPPUNIT_ASSERT( s == "1783456" );

  s = "123456";
  i = s.begin() + 1;
  string::const_iterator ci = s.begin() + 1;
  s.replace(i, i + 3, ci + 3, cs.end());
  CPPUNIT_ASSERT( s == "15656" );

  s = "123456";
  i = s.begin() + 1;
  ci = s.begin() + 1;
  s.replace(i, i + 3, ci, ci + 2);
  CPPUNIT_ASSERT( s == "12356" );

  s = "123456";
  i = s.begin() + 1;
  ci = s.begin() + 1;
  s.replace(i, i + 3, ci + 1, cs.end());
  CPPUNIT_ASSERT( s == "1345656" );
  
  s = "123456";
  s.replace(s.begin() + 4, s.end(), cs.begin(), cs.end());
  CPPUNIT_ASSERT( s == "1234123456" );
  
  /*
   * This is the test for the template replace method.
   */
  s = "123456";
  string::iterator b = s.begin() + 4;
  string::iterator e = s.end();
  string::const_iterator rb = s.begin();
  string::const_iterator re = s.end();
  s.replace(b, e, rb, re);
  CPPUNIT_ASSERT( s == "1234123456" );

  s = "123456";
  s.replace(s.begin() + 4, s.end(), s.begin(), s.end());
  CPPUNIT_ASSERT( s == "1234123456" );
  
  string strorg("This is test string for string calls");
  string str = strorg;
  str.replace(5, 15, str.c_str(), 10);
  CPPUNIT_ASSERT( str == "This This is tefor string calls" );

  str = strorg;
  str.replace(5, 5, str.c_str(), 10);
  CPPUNIT_ASSERT( str == "This This is test string for string calls" );

#if !defined (STLPORT) || defined (_STLP_MEMBER_TEMPLATES)
  deque<char> cdeque;
  cdeque.push_back('I');
  str.replace(str.begin(), str.begin() + 11, cdeque.begin(), cdeque.end());
  CPPUNIT_ASSERT( str == "Is test string for string calls" );
#endif
}

void StringTest::resize()
{
  string s;

  s.resize(0);

  CPPUNIT_ASSERT( *s.c_str() == 0 );

  s = "1234567";

  s.resize(0);
  CPPUNIT_ASSERT( *s.c_str() == 0 );

  s = "1234567";
  s.resize(1);
  CPPUNIT_ASSERT( s.size() == 1 );
  CPPUNIT_ASSERT( *s.c_str() == '1' );
  CPPUNIT_ASSERT( *(s.c_str() + 1) == 0 );

  s = "1234567";
  s.resize(10);
  CPPUNIT_ASSERT( s.size() == 10 );
  CPPUNIT_ASSERT( s[6] == '7' );
  CPPUNIT_ASSERT( s[7] == 0 );
  CPPUNIT_ASSERT( s[8] == 0 );
  CPPUNIT_ASSERT( s[9] == 0 );
}

void StringTest::find()
{
  string s("one two three one two three");
  CPPUNIT_ASSERT( s.find("one") == 0 );
  CPPUNIT_ASSERT( s.find('t') == 4 );
  CPPUNIT_ASSERT( s.find('t', 5) == 8 );
  //We are trying to get a const reference to the npos string static member to
  //force the compiler to allocate memory for this variable. It used to reveal
  //a bug of STLport which was simply declaring npos without instanciating it.
#if !defined (STLPORT) || !defined (_STLP_STATIC_CONST_INIT_BUG)
  string::size_type const& npos_local = string::npos;
#else
#  define npos_local string::npos
#endif
  CPPUNIT_ASSERT( s.find("four") == npos_local );
  CPPUNIT_ASSERT( s.find("one", string::npos) == npos_local );

  CPPUNIT_ASSERT( s.rfind("two") == 18 );
  CPPUNIT_ASSERT( s.rfind("two", 0) == string::npos );
  CPPUNIT_ASSERT( s.rfind("two", 11) == 4 );
  CPPUNIT_ASSERT( s.rfind('w') == 19 );

  CPPUNIT_ASSERT( s.find_first_of("abcde") == 2 );
  CPPUNIT_ASSERT( s.find_last_of("abcde") == 26 );
  
  CPPUNIT_ASSERT( s.find_first_not_of("enotw ") == 9 );
  CPPUNIT_ASSERT( s.find_last_not_of("ehortw ") == 15 );
}

void StringTest::assign()
{
  string s;
  char const* cstr = "test string for assign";

  s.assign(cstr, cstr + 22);
  CPPUNIT_ASSERT( s == "test string for assign" );

  string s2("other test string");
  s.assign(s2);
  CPPUNIT_ASSERT( s == s2 );

  static std::string str1;
  static std::string str2;

  // short string optim:
  str1 = "123456";
  // longer than short string:
  str2 = "1234567890123456789012345678901234567890";

  CPPUNIT_ASSERT(str1[5] == '6');
  CPPUNIT_ASSERT(str2[29] == '0'); 
}

/* This test is to check if std::string properly supports the short string
 * optimization. It has been found out that eMbedded Visual C++ 3.0 and .NET
 * compilers for the ARM platform fail to pass structs and classes of certain
 * size per value. This seems to be a known compiler bug. For other processors
 * (e.g. x86) the error doesn't occur.
 * (The ARM compiler creates a temporary object from teststr on the stack, to
 * pass it to the helper function. It uses the copy constructor for this.
 * After this the temporary object is copied to another place on the stack.
 * The result is that the _M_finish pointer then points to the wrong buffer
 * end and the size of the short string is incorrectly calculated.)
 */
void StringTest::short_string_optim_bug()
{
   string teststr("shortest");

   bool short_string_optim_bug_helper(std::string teststr);

   CPPUNIT_ASSERT(true == short_string_optim_bug_helper(teststr));
}

bool short_string_optim_bug_helper(std::string teststr)
{
   size_t ss = teststr.size();
   return (ss == 8);
} 

void StringTest::compare()
{
  string str1("abcdef");
  string str2;

  str2 = "abcdef";
  CPPUNIT_ASSERT( str1.compare(str2) == 0 );
  str2 = "abcde";
  CPPUNIT_ASSERT( str1.compare(str2) > 0 );
  str2 = "abcdefg";
  CPPUNIT_ASSERT( str1.compare(str2) < 0 );

  CPPUNIT_ASSERT( str1.compare("abcdef") == 0 );
  CPPUNIT_ASSERT( str1.compare("abcde") > 0 );
  CPPUNIT_ASSERT( str1.compare("abcdefg") < 0 );

  str2 = "cde";
  CPPUNIT_ASSERT( str1.compare(2, 3, str2) == 0 );
  str2 = "cd";
  CPPUNIT_ASSERT( str1.compare(2, 3, str2) > 0 );
  str2 = "cdef";
  CPPUNIT_ASSERT( str1.compare(2, 3, str2) < 0 );

  str2 = "abcdef";
  CPPUNIT_ASSERT( str1.compare(2, 3, str2, 2, 3) == 0 );
  CPPUNIT_ASSERT( str1.compare(2, 3, str2, 2, 2) > 0 );
  CPPUNIT_ASSERT( str1.compare(2, 3, str2, 2, 4) < 0 );

  CPPUNIT_ASSERT( str1.compare(2, 3, "cdefgh", 3) == 0 );
  CPPUNIT_ASSERT( str1.compare(2, 3, "cdefgh", 2) > 0 );
  CPPUNIT_ASSERT( str1.compare(2, 3, "cdefgh", 4) < 0 );
}

void StringTest::template_expresion()
{
  string one("one"), two("two"), three("three");
  string space(1, ' ');

  {
    string result(one + ' ' + two + ' ' + three);
    CPPUNIT_CHECK( result == "one two three" );
  }

  {
    string result(one + ' ' + two + ' ' + three, 4);
    CPPUNIT_CHECK( result == "two three" );
  }

  {
    string result(one + ' ' + two + ' ' + three, 4, 3);
    CPPUNIT_CHECK( result == "two" );
  }

  //2 members expressions:
  CPPUNIT_CHECK( (' ' + one) == " one" );
  CPPUNIT_CHECK( (one + ' ') == "one " );
  CPPUNIT_CHECK( (one + " two") == "one two" );
  CPPUNIT_CHECK( ("one " + two) == "one two" );
  CPPUNIT_CHECK( (one + space) == "one " );

  //3 members expressions:
  CPPUNIT_CHECK( ((one + space) + "two") == "one two" );
  CPPUNIT_CHECK( ("one" + (space + two)) == "one two" );
  CPPUNIT_CHECK( ((one + space) + two) == "one two" );
  CPPUNIT_CHECK( (one + (space + two)) == "one two" );
  CPPUNIT_CHECK( ((one + space) + 't') == "one t" );
  CPPUNIT_CHECK( ('o' + (space + two)) == "o two" );

  //4 members expressions:
  CPPUNIT_CHECK( ((one + space) + (two + space)) == "one two " );

  //special operators
  {
    string result;
    result = one + space + two;
    CPPUNIT_CHECK( result == "one two" );

    result += space + three;
    CPPUNIT_CHECK( result == "one two three" );
  }

  //special append method
  {
    string result;
    //Use reserve to avoid reallocation and really test auto-referencing problems:
    result.reserve(64);

    result.append(one + space + two);
    CPPUNIT_CHECK( result == "one two" );

    result.append(space + result + space + three);
    CPPUNIT_CHECK( result == "one two one two three" );

    result = "one two";
    result.append(space + three, 1, 2);
    CPPUNIT_ASSERT( result == "one twoth" );

    result.append(space + result);
    CPPUNIT_CHECK( result == "one twoth one twoth" );
  }

  //special assign method
  {
    string result;
    //Use reserve to avoid reallocation and really test auto-referencing problems:
    result.reserve(64);

    result.assign(one + space + two + space + three);
    CPPUNIT_CHECK( result == "one two three" );

    result.assign(one + space + two + space + three, 3, 5);
    CPPUNIT_CHECK( result == " two " );

    result.assign(one + result + three);
    CPPUNIT_CHECK( result == "one two three" );
  }

  {
    char result;

    CPPUNIT_CHECK( !(one + ' ' + two).empty() );

    result = (one + ' ' + two)[3];
    CPPUNIT_CHECK( result == ' ' );

    result = (one + ' ' + two).at(3);
    CPPUNIT_CHECK( result == ' ' );

#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    for (;;) {
      try {
        result = (one + ' ' + two).at(10);
        CPPUNIT_ASSERT(false);
      }
      catch (out_of_range const&) {
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
}

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
void StringTest::io()
{
  string str("STLport");
  {
    ostringstream ostr;
    ostr << str;
    CPPUNIT_ASSERT( ostr.good() );
    CPPUNIT_ASSERT( ostr.str() == str );
  }
  {
    istringstream istr(str);
    string istr_content;
    istr >> istr_content;
    CPPUNIT_ASSERT( !istr.fail() && istr.eof() );
    CPPUNIT_ASSERT( istr_content == str );
  }
  {
    istringstream istr(str);
    istr.width(3);
    string istr_content;
    istr >> istr_content;
    CPPUNIT_ASSERT( !istr.fail() && !istr.eof() );
    CPPUNIT_ASSERT( istr_content == "STL" );
  }
}
#endif
