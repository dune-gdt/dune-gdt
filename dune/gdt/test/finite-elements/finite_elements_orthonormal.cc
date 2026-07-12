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

#include <dune/geometry/quadraturerules.hh>
#include <dune/geometry/type.hh>

#include <dune/gdt/local/finite-elements/orthonormal.hh>

using namespace Dune;
using namespace Dune::GDT;


// Checks that the basis of a local orthonormal finite element is L2-orthonormal on the reference element, i.e. that
// \int_ref b_i b_j = delta_ij (approximated by a quadrature of sufficiently high order).
template <size_t d>
static void check_orthonormal(const GeometryType& geometry_type, const int order)
{
  auto fe = make_local_orthonormal_finite_element<double, d, double>(geometry_type, order);
  EXPECT_EQ(order, fe->order());
  EXPECT_EQ(geometry_type, fe->geometry_type());
  const auto& basis = fe->basis();
  const size_t sz = basis.size();
  EXPECT_GT(sz, 0u);
  EXPECT_EQ(sz, fe->size());

  std::vector<std::vector<double>> gram(sz, std::vector<double>(sz, 0.));
  const auto quadrature_rule = QuadratureRules<double, d>::rule(geometry_type, 2 * order + 2);
  for (const auto& quadrature_point : quadrature_rule) {
    const auto values = basis.evaluate(quadrature_point.position());
    ASSERT_EQ(sz, values.size());
    const auto weight = quadrature_point.weight();
    for (size_t ii = 0; ii < sz; ++ii)
      for (size_t jj = 0; jj < sz; ++jj)
        gram[ii][jj] += weight * values[ii][0] * values[jj][0];
  }
  for (size_t ii = 0; ii < sz; ++ii)
    for (size_t jj = 0; jj < sz; ++jj)
      EXPECT_NEAR((ii == jj) ? 1. : 0., gram[ii][jj], 1e-12);
} // ... check_orthonormal(...)


GTEST_TEST(finite_elements_orthonormal, line_order_2)
{
  check_orthonormal<1>(GeometryTypes::line, 2);
}

GTEST_TEST(finite_elements_orthonormal, triangle_order_1)
{
  check_orthonormal<2>(GeometryTypes::triangle, 1);
}

GTEST_TEST(finite_elements_orthonormal, triangle_order_2)
{
  check_orthonormal<2>(GeometryTypes::triangle, 2);
}

GTEST_TEST(finite_elements_orthonormal, quadrilateral_order_2)
{
  check_orthonormal<2>(GeometryTypes::quadrilateral, 2);
}

GTEST_TEST(finite_elements_orthonormal, tetrahedron_order_1)
{
  check_orthonormal<3>(GeometryTypes::tetrahedron, 1);
}

GTEST_TEST(finite_elements_orthonormal, hexahedron_order_1)
{
  check_orthonormal<3>(GeometryTypes::hexahedron, 1);
}
