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

#include <dune/gdt/local/finite-elements/lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;


// Checks a scalar (r == 1) local Lagrange finite element for the expected number of DoFs, polynomial order, partition
// of unity and Kronecker delta property at the Lagrange nodes.
template <size_t d>
static void check_lagrange(const GeometryType& geometry_type,
                           const int order,
                           const size_t expected_size,
                           const std::vector<FieldVector<double, d>>& interior_points)
{
  auto fe = make_local_lagrange_finite_element<double, d, double>(geometry_type, order);
  EXPECT_EQ(expected_size, fe->size());
  EXPECT_EQ(order, fe->order());
  EXPECT_EQ(geometry_type, fe->geometry_type());
  EXPECT_TRUE(fe->is_lagrangian());
  const auto& basis = fe->basis();
  EXPECT_EQ(expected_size, basis.size());
  // partition of unity: sum_j basis_j(x) == 1
  for (const auto& point : interior_points) {
    const auto values = basis.evaluate(point);
    ASSERT_EQ(basis.size(), values.size());
    double sum = 0.;
    for (size_t jj = 0; jj < basis.size(); ++jj)
      sum += values[jj][0];
    EXPECT_NEAR(1., sum, 1e-13);
  }
  // Kronecker delta: basis_j(lagrange_point_i) == (i == j ? 1 : 0)
  const auto& lagrange_points = fe->lagrange_points();
  ASSERT_EQ(basis.size(), lagrange_points.size());
  for (size_t ii = 0; ii < lagrange_points.size(); ++ii) {
    const auto values = basis.evaluate(lagrange_points[ii]);
    ASSERT_EQ(basis.size(), values.size());
    for (size_t jj = 0; jj < basis.size(); ++jj)
      EXPECT_NEAR((ii == jj) ? 1. : 0., values[jj][0], 1e-13);
  }
} // ... check_lagrange(...)


GTEST_TEST(finite_elements_lagrange, line_order_1)
{
  check_lagrange<1>(GeometryTypes::line, 1, 2, {{0.3}, {0.7}});
}

GTEST_TEST(finite_elements_lagrange, line_order_2)
{
  check_lagrange<1>(GeometryTypes::line, 2, 3, {{0.3}, {0.7}});
}

GTEST_TEST(finite_elements_lagrange, triangle_order_1)
{
  check_lagrange<2>(GeometryTypes::triangle, 1, 3, {{0.25, 0.25}, {0.2, 0.5}});
}

GTEST_TEST(finite_elements_lagrange, triangle_order_2)
{
  check_lagrange<2>(GeometryTypes::triangle, 2, 6, {{0.25, 0.25}, {0.2, 0.5}});
}

GTEST_TEST(finite_elements_lagrange, quadrilateral_order_1)
{
  check_lagrange<2>(GeometryTypes::quadrilateral, 1, 4, {{0.3, 0.6}, {0.5, 0.5}});
}

GTEST_TEST(finite_elements_lagrange, quadrilateral_order_2)
{
  check_lagrange<2>(GeometryTypes::quadrilateral, 2, 9, {{0.3, 0.6}, {0.5, 0.5}});
}
