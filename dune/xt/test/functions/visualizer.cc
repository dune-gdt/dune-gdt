// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

#include <dune/xt/test/main.hxx> // <- Has to come first, includes the config.h!

#include <cmath>

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/functions/base/visualization.hh>

using namespace Dune;
using namespace Dune::XT::Functions;


GTEST_TEST(visualizer, default_visualizer_vector_valued)
{
  static constexpr size_t r = 3;
  DefaultVisualizer<r, 1> viz;
  EXPECT_EQ(int(r), viz.ncomps());
  FieldVector<double, r> val{1., 2., 3.};
  for (int comp = 0; comp < int(r); ++comp)
    EXPECT_DOUBLE_EQ(val[comp], viz.evaluate(comp, val));
}

GTEST_TEST(visualizer, default_visualizer_matrix_valued)
{
  static constexpr size_t r = 2;
  static constexpr size_t rC = 2;
  DefaultVisualizer<r, rC> viz;
  // matrix-valued functions are visualized via their Frobenius norm in a single component
  EXPECT_EQ(1, viz.ncomps());
  FieldMatrix<double, r, rC> val{{3., 0.}, {0., 4.}};
  EXPECT_DOUBLE_EQ(val.frobenius_norm(), viz.evaluate(0, val));
  EXPECT_DOUBLE_EQ(5., viz.evaluate(0, val));
}

GTEST_TEST(visualizer, default_visualizer_helper)
{
  // the free function returns a usable reference to a static instance
  static constexpr size_t r = 2;
  const auto& viz = default_visualizer<r, 1>();
  EXPECT_EQ(int(r), viz.ncomps());
  FieldVector<double, r> val{7., 9.};
  EXPECT_DOUBLE_EQ(7., viz.evaluate(0, val));
  EXPECT_DOUBLE_EQ(9., viz.evaluate(1, val));
}

GTEST_TEST(visualizer, sum_visualizer)
{
  static constexpr size_t r = 4;
  SumVisualizer<r> viz;
  EXPECT_EQ(1, viz.ncomps());
  FieldVector<double, r> val{1., 2., 3., 4.};
  EXPECT_DOUBLE_EQ(10., viz.evaluate(0, val));
}

GTEST_TEST(visualizer, component_visualizer)
{
  static constexpr size_t r = 3;
  const int component = 1;
  ComponentVisualizer<r> viz(component);
  EXPECT_EQ(1, viz.ncomps());
  FieldVector<double, r> val{10., 20., 30.};
  EXPECT_DOUBLE_EQ(20., viz.evaluate(0, val));
  // it only ever plots a single component, so anything but comp == 0 is an error
  EXPECT_THROW(viz.evaluate(1, val), Dune::InvalidStateException);
}

GTEST_TEST(visualizer, generic_visualizer)
{
  static constexpr size_t r = 3;
  using RangeType = typename VisualizerInterface<r, 1>::RangeType;
  GenericVisualizer<r> viz(2, [](const int comp, const RangeType& val) { return val[comp] * (comp + 1); });
  EXPECT_EQ(2, viz.ncomps());
  RangeType val{5., 6., 7.};
  EXPECT_DOUBLE_EQ(5., viz.evaluate(0, val));
  EXPECT_DOUBLE_EQ(12., viz.evaluate(1, val));
}
