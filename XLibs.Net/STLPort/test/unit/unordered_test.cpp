#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class UnorderedTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(UnorderedTest);
  CPPUNIT_TEST(uset);
  CPPUNIT_TEST(umultiset);
  CPPUNIT_TEST(umap);
  CPPUNIT_TEST(umultimap);
  CPPUNIT_TEST(user_case);
  CPPUNIT_TEST(hash_policy);
  CPPUNIT_TEST(buckets);
  CPPUNIT_TEST_SUITE_END();

protected:
  void uset();
  void umultiset();
  void umap();
  void umultimap();
  void user_case();
  void hash_policy();
  void buckets();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnorderedTest);

const int NB_ELEMS = 2000;

//
// tests implementation
//
void UnorderedTest::uset()
{
  typedef unordered_set<int, hash<int>, equal_to<int> > usettype;
  usettype us;

  //Small compilation check of the copy constructor:
  usettype us2(us);
  //And assignment operator
  us = us2;

  int i;
  pair<usettype::iterator, bool> ret;
  for (i = 0; i < NB_ELEMS; ++i) {
    ret = us.insert(i);
    CPPUNIT_ASSERT( ret.second );
    CPPUNIT_ASSERT( *ret.first == i );

    ret = us.insert(i);
    CPPUNIT_ASSERT( !ret.second );
    CPPUNIT_ASSERT( *ret.first == i );
  }

  vector<int> us_val;

  usettype::local_iterator lit, litEnd;
  for (i = 0; i < NB_ELEMS; ++i) {
    lit = us.begin(us.bucket(i));
    litEnd = us.end(us.bucket(i));

    usettype::size_type bucket_pos = us.bucket(*lit);
    for (; lit != litEnd; ++lit) {
      CPPUNIT_ASSERT( us.bucket(*lit) == bucket_pos );
      us_val.push_back(*lit);
    }
  }

  //A compilation time check to uncomment from time to time
  {
    //usettype::iterator it;
    //CPPUNIT_ASSERT( it != lit );
  }

  sort(us_val.begin(), us_val.end());
  for (i = 0; i < NB_ELEMS; ++i) {
    CPPUNIT_ASSERT( us_val[i] == i );
  }
}

void UnorderedTest::umultiset()
{
  typedef unordered_multiset<int, hash<int>, equal_to<int> > usettype;
  usettype us;

  int i;
  usettype::iterator ret;
  for (i = 0; i < NB_ELEMS; ++i) {
    ret = us.insert(i);
    CPPUNIT_ASSERT( *ret == i );

    ret = us.insert(i);
    CPPUNIT_ASSERT( *ret == i );
  }

  CPPUNIT_ASSERT( us.size() == 2 * NB_ELEMS );
  vector<int> us_val;

  usettype::local_iterator lit, litEnd;
  for (i = 0; i < NB_ELEMS; ++i) {
    lit = us.begin(us.bucket(i));
    litEnd = us.end(us.bucket(i));

    usettype::size_type bucket_pos = us.bucket(*lit);
    for (; lit != litEnd; ++lit) {
      CPPUNIT_ASSERT( us.bucket(*lit) == bucket_pos );
      us_val.push_back(*lit);
    }
  }

  sort(us_val.begin(), us_val.end());
  for (i = 0; i < NB_ELEMS; ++i) {
    CPPUNIT_ASSERT( us_val[2 * i] == i );
    CPPUNIT_ASSERT( us_val[2 * i + 1] == i );
  }
}

void UnorderedTest::umap()
{
  typedef unordered_map<int, int, hash<int>, equal_to<int> > umaptype;
  umaptype us;

  //Compilation check of the [] operator:
  umaptype us2;
  us[0] = us2[0];
  us.clear();

  {
    //An other compilation check
    typedef unordered_map<int, umaptype> uumaptype;
    uumaptype uus;
    umaptype const& uref = uus[0];
    umaptype ucopy = uus[0];
    ucopy = uref;
    //Avoids warning:
    //(void*)&uref;
  }

  int i;
  pair<umaptype::iterator, bool> ret;
  for (i = 0; i < NB_ELEMS; ++i) {
    pair<const int, int> p1(i, i);
    ret = us.insert(p1);
    CPPUNIT_ASSERT( ret.second );
    CPPUNIT_ASSERT( *ret.first == p1 );

    pair<const int, int> p2(i, i + 1);
    ret = us.insert(p2);
    CPPUNIT_ASSERT( !ret.second );
    CPPUNIT_ASSERT( *ret.first == p1 );
  }

  {
    //Lets look for some values to see if everything is normal:
    umaptype::iterator umit;
    for (int j = 0; j < NB_ELEMS; j += NB_ELEMS / 100) {
      umit = us.find(j);

      CPPUNIT_ASSERT( umit != us.end() );
      CPPUNIT_ASSERT( (*umit).second == j );
    }
  }

  CPPUNIT_ASSERT( us.size() == (size_t)NB_ELEMS );
  vector<pair<int, int> > us_val;

  umaptype::local_iterator lit, litEnd;
  for (i = 0; i < NB_ELEMS; ++i) {
    lit = us.begin(us.bucket(i));
    litEnd = us.end(us.bucket(i));

    umaptype::size_type bucket_pos = us.bucket((*lit).first);
    for (; lit != litEnd; ++lit) {
      CPPUNIT_ASSERT( us.bucket((*lit).first) == bucket_pos );
      us_val.push_back(*lit);
    }
  }

  sort(us_val.begin(), us_val.end());
  for (i = 0; i < NB_ELEMS; ++i) {
    CPPUNIT_ASSERT( us_val[i] == make_pair(i, i) );
  }
}

void UnorderedTest::umultimap()
{
  typedef unordered_multimap<int, int, hash<int>, equal_to<int> > umaptype;
  umaptype us;

  int i;
  umaptype::iterator ret;
  for (i = 0; i < NB_ELEMS; ++i) {
    pair<const int, int> p(i, i);
    ret = us.insert(p);
    CPPUNIT_ASSERT( *ret == p );

    ret = us.insert(p);
    CPPUNIT_ASSERT( *ret == p );
  }

  CPPUNIT_ASSERT( us.size() == 2 * NB_ELEMS );
  typedef pair<int, int> ptype;
  vector<ptype> us_val;

  umaptype::local_iterator lit, litEnd;
  for (i = 0; i < NB_ELEMS; ++i) {
    lit = us.begin(us.bucket(i));
    litEnd = us.end(us.bucket(i));

    umaptype::size_type bucket_pos = us.bucket((*lit).first);
    for (; lit != litEnd; ++lit) {
      CPPUNIT_ASSERT( us.bucket((*lit).first) == bucket_pos );
      us_val.push_back(ptype((*lit).first, (*lit).second));
    }
  }

  sort(us_val.begin(), us_val.end());
  for (i = 0; i < NB_ELEMS; ++i) {
    pair<int, int> p(i, i);
    CPPUNIT_ASSERT( us_val[i * 2] == p );
    CPPUNIT_ASSERT( us_val[i * 2 + 1] == p );
  }
}

void UnorderedTest::user_case()
{
  typedef unordered_map<int, string> UnorderedMap1;
  typedef unordered_map<int, UnorderedMap1> UnorderedMap2;

  UnorderedMap1 foo;
  UnorderedMap2 bar;

  foo.insert(make_pair(1, string("test1")));
  foo.insert(make_pair(2, string("test2")));
  foo.insert(make_pair(3, string("test3")));
  foo.insert(make_pair(4, string("test4")));
  foo.insert(make_pair(5, string("test5")));

  bar.insert(make_pair(0, foo));
  UnorderedMap2::iterator it = bar.find(0);
  CPPUNIT_ASSERT( it != bar.end() );

  UnorderedMap1 &body = it->second;
  UnorderedMap1::iterator cur = body.find(3);
  CPPUNIT_ASSERT( cur != body.end() );

  body.erase(body.begin(), body.end());
  CPPUNIT_ASSERT( body.empty() );
}

void UnorderedTest::hash_policy()
{
  unordered_set<int> int_uset;

  CPPUNIT_ASSERT( int_uset.max_load_factor() == 1.0f );
  CPPUNIT_ASSERT( int_uset.load_factor() == 0.0f );

  size_t nbInserts = int_uset.bucket_count() - 1;
  for (int i = 0; (size_t)i < nbInserts; ++i) {
    int_uset.insert(i);
  }
  CPPUNIT_ASSERT( int_uset.size() == nbInserts );

  int_uset.max_load_factor(0.5f);
  int_uset.rehash(0);
  CPPUNIT_ASSERT( int_uset.load_factor() < int_uset.max_load_factor() );

  size_t bucketsHint = int_uset.bucket_count() + 1;
  int_uset.rehash(bucketsHint);
  CPPUNIT_ASSERT( int_uset.bucket_count() >= bucketsHint );
}

void UnorderedTest::buckets()
{
  unordered_set<int> int_uset;

  CPPUNIT_ASSERT( int_uset.bucket_count() < int_uset.max_bucket_count() );

  int i;
  size_t nbBuckets = int_uset.bucket_count();
  size_t nbInserts = int_uset.bucket_count() - 1;
  for (i = 0; (size_t)i < nbInserts; ++i) {
    int_uset.insert(i);
  }
  CPPUNIT_ASSERT( nbBuckets == int_uset.bucket_count() );

  size_t bucketSizes = 0;
  for (i = 0; (size_t)i < nbBuckets; ++i) {
    bucketSizes += int_uset.bucket_size(i);
  }
  CPPUNIT_ASSERT( bucketSizes == int_uset.size() );
}
