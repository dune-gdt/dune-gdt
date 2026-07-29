// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/fix-ambiguous-std-math-overloads.hh/.cc, i.e. the std:: overloads which make
// dune/common/densevector.hh usable with unsigned field types and with Dune::bigunsignedint.
//
// The header is pulled in by config.h (see the bottom of config.h.cmake), so main.hxx already provides it; it is
// included explicitly below anyway to document what is under test.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cstdint>
#include <limits>

#include <dune/common/bigunsignedint.hh>
#include <dune/common/dynvector.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/fix-ambiguous-std-math-overloads.hh>

using BigUInt = Dune::bigunsignedint<64>;


GTEST_TEST(fix_ambiguous_std_math_overloads, abs_of_unsigned_integers_is_the_identity)
{
  // Both of these are defined in the .cc; without them std::abs would be ambiguous for these types. That they have
  // no effect is exactly the point, so clang's warning about it is of no interest here.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wabsolute-value"
#endif
  const long unsigned int zero = 0ul;
  const long unsigned int large = std::numeric_limits<long unsigned int>::max();
  EXPECT_EQ(zero, std::abs(zero));
  EXPECT_EQ(42ul, std::abs(42ul));
  EXPECT_EQ(large, std::abs(large));

  const unsigned char small = 0;
  EXPECT_EQ(small, std::abs(small));
  EXPECT_EQ(static_cast<unsigned char>(200), std::abs(static_cast<unsigned char>(200)));
  EXPECT_EQ(std::numeric_limits<unsigned char>::max(), std::abs(std::numeric_limits<unsigned char>::max()));
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
}


GTEST_TEST(fix_ambiguous_std_math_overloads, abs_and_conj_of_bigunsignedint_are_the_identity)
{
  const BigUInt value(1234567);
  EXPECT_EQ(value, std::abs(value));
  EXPECT_EQ(value, std::conj(value));
  EXPECT_EQ(BigUInt(0), std::abs(BigUInt(0)));
}


GTEST_TEST(fix_ambiguous_std_math_overloads, bigunsignedint_is_never_nan_or_inf)
{
  EXPECT_FALSE(std::isnan(BigUInt(0)));
  EXPECT_FALSE(std::isnan(BigUInt(1234567)));
  EXPECT_FALSE(std::isinf(BigUInt(0)));
  EXPECT_FALSE(std::isinf(BigUInt(1234567)));
}


GTEST_TEST(fix_ambiguous_std_math_overloads, pow_and_sqrt_of_bigunsignedint_are_not_implemented)
{
  EXPECT_THROW(std::pow(BigUInt(4), std::uintmax_t(2)), Dune::NotImplemented);
  EXPECT_THROW(std::sqrt(BigUInt(4)), Dune::NotImplemented);
}


GTEST_TEST(fix_ambiguous_std_math_overloads, dense_vectors_of_unsigned_field_types_can_compute_their_norms)
{
  // This is what the whole header exists for: without the overloads above the one_norm() below does not compile.
  Dune::DynamicVector<long unsigned int> dynamic(3, 2ul);
  EXPECT_EQ(6ul, dynamic.one_norm());
  EXPECT_EQ(2ul, dynamic.infinity_norm());

  Dune::FieldVector<long unsigned int, 2> fixed(3ul);
  EXPECT_EQ(6ul, fixed.one_norm());
  EXPECT_EQ(3ul, fixed.infinity_norm());
}
