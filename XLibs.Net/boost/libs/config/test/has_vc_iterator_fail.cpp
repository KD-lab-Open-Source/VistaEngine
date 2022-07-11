
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

// Test file for macro BOOST_MSVC_STD_ITERATOR
// This file should not compile, if it does then
// BOOST_MSVC_STD_ITERATOR may be defined.
// see boost_has_vc_iterator.cxx for more details

// Do not edit this file, it was generated automatically by
// ../tools/generate from boost_has_vc_iterator.cxx on
// Tue Oct  7 11:26:18 GMTST 2003

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifndef BOOST_MSVC_STD_ITERATOR
#include "boost_has_vc_iterator.cxx"
#else
#error "this file should not compile"
#endif

int cpp_main( int, char *[] )
{
   return boost_msvc_std_iterator::test();
}  
   
