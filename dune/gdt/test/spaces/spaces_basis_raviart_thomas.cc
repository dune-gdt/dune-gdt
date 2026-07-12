// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/geometry/referenceelements.hh>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/spaces/hdiv/raviart-thomas.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using D = typename G::ctype;


// Mirrors basis_exists_on_each_element_with_correct_size / _order from spaces_hdiv_raviart_thomas.cc: the localized RT0
// basis has one shape function per intersection of the element (reference_element.size(1), i.e. 4 on a quadrilateral)
// and its shape functions are degree-1 polynomials, so order() == 1.
GTEST_TEST(spaces_basis_raviart_thomas, basis_has_correct_size_and_order)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_raviart_thomas_space(grid_view, 0);

  for (auto&& element : Dune::elements(grid_view)) {
    const auto& reference_element = ReferenceElements<D, d>::general(element.type());
    const auto basis = space.basis().localize(element);
    EXPECT_GT(basis->size(), 0u);
    EXPECT_EQ(static_cast<size_t>(reference_element.size(1)), basis->size());
    EXPECT_EQ(1, basis->order());
  }
}


// The RT space is vector-valued: each basis function evaluates to a d-dimensional vector.
GTEST_TEST(spaces_basis_raviart_thomas, basis_is_vector_valued)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_raviart_thomas_space(grid_view, 0);

  for (auto&& element : Dune::elements(grid_view)) {
    const auto& reference_element = ReferenceElements<D, d>::general(element.type());
    const auto basis = space.basis().localize(element);
    const auto values = basis->evaluate_set(reference_element.position(0, 0));
    ASSERT_EQ(basis->size(), values.size());
    for (size_t ii = 0; ii < values.size(); ++ii)
      EXPECT_EQ(d, values[ii].size());
  }
}
