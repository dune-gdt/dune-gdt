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

#include <dune/gdt/local/finite-elements/lagrange.hh>
#include <dune/gdt/local/finite-elements/power.hh>

using namespace Dune;
using namespace Dune::GDT;


GTEST_TEST(finite_elements_power, size_and_block_structure)
{
  // scalar (r == 1) order 1 Lagrange finite element on the triangle
  auto scalar_fe = make_local_lagrange_finite_element<double, 2, double>(GeometryTypes::triangle, 1);
  const size_t base_size = scalar_fe->size();
  EXPECT_EQ(3u, base_size);

  // wrap it into a power finite element with power == 2, yielding r == 2
  static constexpr size_t power = 2;
  auto powered = make_local_powered_finite_element<power>(*scalar_fe);
  EXPECT_EQ(power * base_size, powered->size());
  EXPECT_EQ(scalar_fe->order(), powered->order());
  EXPECT_EQ(scalar_fe->geometry_type(), powered->geometry_type());

  const auto& scalar_basis = scalar_fe->basis();
  const auto& power_basis = powered->basis();
  EXPECT_EQ(power * base_size, power_basis.size());

  FieldVector<double, 2> point;
  point[0] = 0.25;
  point[1] = 0.25;
  const auto scalar_values = scalar_basis.evaluate(point); // std::vector<FieldVector<double, 1>>
  const auto power_values = power_basis.evaluate(point); // std::vector<FieldVector<double, 2>>
  ASSERT_EQ(base_size, scalar_values.size());
  ASSERT_EQ(power * base_size, power_values.size());

  // The power basis is block-diagonal: the pp-th copy of the scalar basis function ii lives in component pp of the
  // range and is zero in every other component (see LocalPowerFiniteElementBasis::evaluate).
  for (size_t ii = 0; ii < base_size; ++ii) {
    // first copy (component 0)
    EXPECT_NEAR(scalar_values[ii][0], power_values[ii][0], 1e-14);
    EXPECT_NEAR(0., power_values[ii][1], 1e-14);
    // second copy (component 1)
    EXPECT_NEAR(0., power_values[base_size + ii][0], 1e-14);
    EXPECT_NEAR(scalar_values[ii][0], power_values[base_size + ii][1], 1e-14);
  }
}
