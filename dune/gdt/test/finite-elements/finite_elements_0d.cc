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

#include <dune/gdt/local/finite-elements/0d.hh>

using namespace Dune;
using namespace Dune::GDT;


GTEST_TEST(finite_elements_0d, size_order_and_geometry_type)
{
  Local0dFiniteElement<double, double> fe;
  EXPECT_EQ(1u, fe.size());
  EXPECT_EQ(0, fe.order());
  EXPECT_EQ(GeometryTypes::simplex(0), fe.geometry_type());
  EXPECT_EQ(1u, fe.basis().size());
  EXPECT_EQ(1u, fe.coefficients().size());
  EXPECT_EQ(1u, fe.interpolation().size());
}

GTEST_TEST(finite_elements_0d, single_basis_is_one)
{
  Local0dFiniteElement<double, double> fe;
  const auto& basis = fe.basis();
  const FieldVector<double, 0> point;
  const auto values = basis.evaluate(point);
  ASSERT_EQ(1u, values.size());
  EXPECT_NEAR(1., values[0][0], 1e-15);
}
