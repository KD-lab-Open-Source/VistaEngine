// -------------------------------------
// integer_log2.hpp
//
//   Gives the integer part of the logarithm, in base 2, of a
// given number. Behavior is undefined if the argument is <= 0.
//
//
//       (C) Copyright Gennaro Prota 2003 - 2004.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// ------------------------------------------------------



#ifndef BOOST_INTEGER_LOG2_HPP_GP_20030301
#define BOOST_INTEGER_LOG2_HPP_GP_20030301

#include <cassert>
#include "boost/limits.hpp"
#include "boost/config.hpp"


namespace boost {
 namespace detail {

  template <typename T>
  int integer_log2_impl(T x, int n) {

      int result = 0;

      while (x != 1) {

          const T t = x >> n;
          if (t) {
              result += n;
              x = t;
          }
          n /= 2;

      }

      return result;
  }



  // helper to find the maximum power of two
  // less than p (more involved than necessary,
  // to avoid PTS)
  //
  template <int p, int n>
  struct max_pow2_less {

      enum { c = 2*n < p };

      BOOST_STATIC_CONSTANT(int, value =
          c ? (max_pow2_less< c*p, 2*c*n>::value) : n);

  };

  template <>
  struct max_pow2_less<0, 0> {

      BOOST_STATIC_CONSTANT(int, value = 0);
  };

 } // detail


 // ---------
 // integer_log2
 // ---------------
 //
 template <typename T>
 int integer_log2(T x) {

     assert(x > 0);

     const int n = detail::max_pow2_less<
                    std::numeric_limits<T> :: digits, 4
                    > :: value;

     return detail::integer_log2_impl(x, n);

 }



}



#endif // include guard
