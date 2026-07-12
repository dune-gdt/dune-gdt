// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/common/fvector.hh>

#include <dune/geometry/type.hh>

#include <dune/gdt/local/finite-elements/raviart-thomas.hh>

using namespace Dune;
using namespace Dune::GDT;


// Checks a lowest order (order 0) local Raviart-Thomas finite element: its number of DoFs, its (finite element) order
// and that its basis is vector-valued with range dimension d.
template <size_t d>
static void check_rt0(const GeometryType& geometry_type,
                      const size_t expected_size,
                      const FieldVector<double, d>& interior_point)
{
  auto fe = make_local_raviart_thomas_finite_element<double, d, double>(geometry_type, 0);
  EXPECT_EQ(expected_size, fe->size());
  EXPECT_EQ(0, fe->order());
  EXPECT_EQ(geometry_type, fe->geometry_type());
  const auto& basis = fe->basis();
  EXPECT_EQ(expected_size, basis.size());
  const auto values = basis.evaluate(interior_point); // std::vector<FieldVector<double, d>>
  ASSERT_EQ(basis.size(), values.size());
  // each basis function is vector-valued with range dimension d
  for (size_t ii = 0; ii < values.size(); ++ii)
    EXPECT_EQ(static_cast<size_t>(d), values[ii].size());
}


GTEST_TEST(finite_elements_raviart_thomas, order_0_triangle)
{
  check_rt0<2>(GeometryTypes::triangle, 3, {0.25, 0.25});
}

GTEST_TEST(finite_elements_raviart_thomas, order_0_quadrilateral)
{
  check_rt0<2>(GeometryTypes::quadrilateral, 4, {0.5, 0.5});
}

GTEST_TEST(finite_elements_raviart_thomas, order_0_tetrahedron)
{
  check_rt0<3>(GeometryTypes::tetrahedron, 4, {0.25, 0.25, 0.25});
}

GTEST_TEST(finite_elements_raviart_thomas, order_0_hexahedron)
{
  check_rt0<3>(GeometryTypes::hexahedron, 6, {0.5, 0.5, 0.5});
}
