// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <sstream>
#include <string>

#include <dune/xt/grid/grids.hh>

#include <dune/gdt/local/finite-elements/interfaces.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/type_traits.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


template <class T>
static std::string streamed(const T& t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}


GTEST_TEST(type_traits, space_type_streaming)
{
  EXPECT_EQ("continuous_lagrange", streamed(SpaceType::continuous_lagrange));
  EXPECT_EQ("continuous_flattop", streamed(SpaceType::continuous_flattop));
  EXPECT_EQ("discontinuous_lagrange", streamed(SpaceType::discontinuous_lagrange));
  EXPECT_EQ("finite_volume", streamed(SpaceType::finite_volume));
  EXPECT_EQ("finite_volume_skeleton", streamed(SpaceType::finite_volume_skeleton));
  EXPECT_EQ("raviart_thomas", streamed(SpaceType::raviart_thomas));
  // unknown values fall back to a generic representation
  EXPECT_EQ("SpaceType(-1)", streamed(static_cast<SpaceType>(-1)));
}


GTEST_TEST(type_traits, stencil_streaming)
{
  EXPECT_EQ("element", streamed(Stencil::element));
  EXPECT_EQ("intersection", streamed(Stencil::intersection));
  EXPECT_EQ("element_and_intersection", streamed(Stencil::element_and_intersection));
  EXPECT_EQ("automatic", streamed(Stencil::automatic));
  // unknown values fall back to a generic representation
  EXPECT_EQ("Stencil(-1)", streamed(static_cast<Stencil>(-1)));
}


GTEST_TEST(type_traits, is_space)
{
  using SpaceT = ContinuousLagrangeSpace<GV, 1, double>;
  EXPECT_TRUE(is_space<SpaceT>::value);
  EXPECT_FALSE(is_space<double>::value);
  EXPECT_FALSE(is_space<int>::value);
}


GTEST_TEST(type_traits, is_local_finite_element)
{
  using LFE = LocalFiniteElementInterface<double, 2, double, 1, 1>;
  EXPECT_TRUE(is_local_finite_element<LFE>::value);
  EXPECT_FALSE(is_local_finite_element<double>::value);

  using LFEFamily = LocalFiniteElementFamilyInterface<double, 2, double, 1, 1>;
  EXPECT_TRUE(is_local_finite_element_family<LFEFamily>::value);
  EXPECT_FALSE(is_local_finite_element_family<double>::value);
}
