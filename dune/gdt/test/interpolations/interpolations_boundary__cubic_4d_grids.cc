// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)
//   Tobias Leibner  (2019 - 2020)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/grid/grids.hh>

#include "boundary.hh"


using Cubic4dGrids = ::testing::Types<YASP_4D_EQUIDISTANT_OFFSET>;


template <class G>
using BoundaryInterpolation = Dune::GDT::Test::BoundaryInterpolationOnLeafViewTest<G>;
TYPED_TEST_SUITE(BoundaryInterpolation, Cubic4dGrids);
TYPED_TEST(BoundaryInterpolation, interpolates_correctly)
{
  this->interpolates_correctly();
}
