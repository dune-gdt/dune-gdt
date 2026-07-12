// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/grid/grids.hh>

#include "raviart-thomas.hh"


using Cubic2dGrids = ::testing::Types<YASP_2D_EQUIDISTANT_OFFSET
#if HAVE_DUNE_ALUGRID
                                      ,
                                      ALU_2D_CUBE
#endif
#if HAVE_DUNE_UGGRID
                                      ,
                                      UG_2D
#endif
                                      >;


template <class G>
using RaviartThomasInterpolationTest = Dune::GDT::Test::RaviartThomasInterpolationOnLeafViewTest<G>;
TYPED_TEST_SUITE(RaviartThomasInterpolationTest, Cubic2dGrids);
TYPED_TEST(RaviartThomasInterpolationTest, interpolates_constant_field_correctly)
{
  this->interpolates_constant_field_correctly();
}
TYPED_TEST(RaviartThomasInterpolationTest, all_overloads_reproduce_constant_field)
{
  this->all_overloads_reproduce_constant_field();
}
TYPED_TEST(RaviartThomasInterpolationTest, interpolant_has_continuous_normal_components)
{
  this->interpolant_has_continuous_normal_components();
}
