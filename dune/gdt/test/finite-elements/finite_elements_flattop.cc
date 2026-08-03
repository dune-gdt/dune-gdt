// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <vector>

#include <dune/common/fvector.hh>

#include <dune/geometry/type.hh>

#include <dune/gdt/local/finite-elements/flattop.hh>

using namespace Dune;
using namespace Dune::GDT;


GTEST_TEST(finite_elements_flattop, size_and_order)
{
  auto fe = make_local_flattop_finite_element<double, 2, double>(GeometryTypes::quadrilateral, 1);
  EXPECT_EQ(4u, fe->size());
  // the order of the finite element is 1, while the polynomial degree of its basis is 2
  EXPECT_EQ(1, fe->order());
  EXPECT_EQ(GeometryTypes::quadrilateral, fe->geometry_type());
  const auto& basis = fe->basis();
  EXPECT_EQ(4u, basis.size());
  EXPECT_EQ(2, basis.order());
}

GTEST_TEST(finite_elements_flattop, basis_values)
{
  auto fe = make_local_flattop_finite_element<double, 2, double>(GeometryTypes::quadrilateral, 1);
  const auto& basis = fe->basis();

  // Each shape function is associated with one corner of the reference cube and equals one there and zero at the other
  // corners (with the default overlap of 0.5).
  const std::vector<FieldVector<double, 2>> corners = {{0., 0.}, {1., 0.}, {0., 1.}, {1., 1.}};
  for (size_t corner_index = 0; corner_index < corners.size(); ++corner_index) {
    const auto values = basis.evaluate(corners[corner_index]);
    ASSERT_EQ(4u, values.size());
    for (size_t jj = 0; jj < 4; ++jj)
      EXPECT_NEAR((corner_index == jj) ? 1. : 0., values[jj][0], 1e-13);
  }

  // partition of unity at some interior points
  const std::vector<FieldVector<double, 2>> interior_points = {{0.5, 0.5}, {0.3, 0.7}, {0.1, 0.9}};
  for (const auto& point : interior_points) {
    const auto values = basis.evaluate(point);
    ASSERT_EQ(4u, values.size());
    double sum = 0.;
    for (size_t jj = 0; jj < 4; ++jj)
      sum += values[jj][0];
    EXPECT_NEAR(1., sum, 1e-13);
  }
}
