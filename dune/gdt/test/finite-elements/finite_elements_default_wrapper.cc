// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/geometry/type.hh>

#include <dune/gdt/local/finite-elements/lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;


// The local Lagrange finite element is a LocalFiniteElementWrapper (wrapper.hh) around a dune-localfunctions
// LagrangeLocalFiniteElement, which in turn derives from LocalFiniteElementDefault (default.hh). This test exercises the
// generic accessors provided by these two classes.
GTEST_TEST(finite_elements_default_wrapper, generic_accessors)
{
  auto fe = make_local_lagrange_finite_element<double, 2, double>(GeometryTypes::triangle, 2);
  EXPECT_EQ(6u, fe->size());
  EXPECT_EQ(2, fe->order());
  EXPECT_EQ(GeometryTypes::triangle, fe->geometry_type());
  EXPECT_TRUE(fe->is_lagrangian());
  EXPECT_FALSE(fe->powered());

  const auto& basis = fe->basis();
  const auto& coefficients = fe->coefficients();
  const auto& interpolation = fe->interpolation();

  // sizes of the three ingredients agree with the finite element size
  EXPECT_EQ(fe->size(), basis.size());
  EXPECT_EQ(fe->size(), coefficients.size());
  EXPECT_EQ(fe->size(), interpolation.size());

  // all ingredients live on the same reference element
  EXPECT_EQ(GeometryTypes::triangle, basis.geometry_type());
  EXPECT_EQ(GeometryTypes::triangle, coefficients.geometry_type());
  EXPECT_EQ(GeometryTypes::triangle, interpolation.geometry_type());

  // the wrapped basis reports the polynomial order of the dune-localfunctions implementation
  EXPECT_EQ(2, basis.order());
}

GTEST_TEST(finite_elements_default_wrapper, coefficients_local_keys)
{
  auto fe = make_local_lagrange_finite_element<double, 2, double>(GeometryTypes::triangle, 2);
  const auto& coefficients = fe->coefficients();

  // every local key is associated with a subentity of codimension in {0, ..., d}
  for (size_t ii = 0; ii < coefficients.size(); ++ii) {
    const auto& local_key = coefficients.local_key(ii);
    EXPECT_LE(local_key.codim(), 2u);
  }

  // the reverse map has one entry per codimension
  const auto local_key_indices = coefficients.local_key_indices();
  EXPECT_EQ(3u, local_key_indices.size());
}
