// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/grid/boundaryinfo/allneumann.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/interpolations/default.hh>
#include <dune/gdt/operators/oswald-interpolation.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using I = XT::Grid::extract_intersection_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


/// The operator documents that it clears exactly those range DoFs associated with its assembly grid view (so that a
/// range vector may be reused/prefilled). The clearing loop used to iterate over the *values* of the DoF-to-Lagrange-
/// point map (i.e. Lagrange point ids) instead of its indices (DoF ids), so it zeroed the wrong entries and stale
/// values survived in the result.
GTEST_TEST(OswaldInterpolationOperator, clears_prefilled_range_vectors)
{
  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 8u);
  auto grid_view = grid_provider.leaf_view();
  const auto space = make_discontinuous_lagrange_space(grid_view, /*order=*/1);
  const XT::Grid::AllNeumannBoundaryInfo<I> boundary_info;
  OswaldInterpolationOperator<GV> oswald_interpolation(grid_view, space, boundary_info);
  oswald_interpolation.assemble();

  // a discontinuous source
  const auto source =
      default_interpolation<V>(0, [](const auto& xx, const auto& /*mu*/) { return xx[0] < 0.5 ? 0. : 1.; }, space);

  // reference: apply into a fresh (zero-initialized) vector
  V fresh_range(space.mapper().size(), 0.);
  oswald_interpolation.apply(source, fresh_range);

  // the same apply into a prefilled vector has to yield the same result
  V prefilled_range(space.mapper().size(), 777.);
  oswald_interpolation.apply(source, prefilled_range);

  for (size_t ii = 0; ii < space.mapper().size(); ++ii)
    EXPECT_DOUBLE_EQ(prefilled_range[ii], fresh_range[ii]) << "ii = " << ii;
}
