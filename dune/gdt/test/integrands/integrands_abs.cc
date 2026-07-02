// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/gdt/local/integrands/abs.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct AbsIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;

  // Scalar abs integrand (r=1): two_norm of a scalar is just |value|
  using ScalarAbsIntegrand = LocalElementAbsIntegrand<E, 1>;
  // Vector abs integrand (r=2): two_norm of a 2-vector
  using VectorAbsIntegrand = LocalElementAbsIntegrand<E, 2>;

  void is_constructable() final
  {
    [[maybe_unused]] ScalarAbsIntegrand scalar_integrand;
    [[maybe_unused]] VectorAbsIntegrand vector_integrand;
    // copy construction
    [[maybe_unused]] ScalarAbsIntegrand scalar_copy(scalar_integrand);
    [[maybe_unused]] VectorAbsIntegrand vector_copy(vector_integrand);
  }

  void order_equals_basis_order()
  {
    ScalarAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    // scalar_test_ has order 4
    EXPECT_EQ(scalar_test_->order(), integrand.order(*scalar_test_));
    // scalar_ansatz_ has order 3
    EXPECT_EQ(scalar_ansatz_->order(), integrand.order(*scalar_ansatz_));
  }

  void scalar_abs_equals_absolute_value_of_basis()
  {
    // scalar_test_ = {x[1], x[0]*x[1]^3}
    // two_norm of a scalar value v is |v| (since FieldVector<double,1>::two_norm = abs)
    // So result[0] = |x[1]|, result[1] = |x[0]*x[1]^3|
    // On the reference element all coordinates are in [0,1], so values are non-negative already.
    ScalarAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);

    const auto order = integrand.order(*scalar_test_);
    DynamicVector<double> result(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, x, result);
      // scalar_test_: phi_0(x) = x[1], phi_1(x) = x[0]*x[1]^3
      EXPECT_NEAR(std::abs(x[1]), result[0], 1e-13);
      EXPECT_NEAR(std::abs(x[0] * std::pow(x[1], 3)), result[1], 1e-13);
    }
  }

  void vector_abs_equals_two_norm_of_basis()
  {
    // vector_test_ = {(1,2)^T, (x[0]*x[1], x[0]+x[1])^T}
    // two_norm of (a,b)^T = sqrt(a^2 + b^2)
    // So result[0] = ||(1,2)^T|| = sqrt(5)
    //    result[1] = ||(x[0]*x[1], x[0]+x[1])^T|| = sqrt((x[0]*x[1])^2 + (x[0]+x[1])^2)
    VectorAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);

    const auto order = integrand.order(*vector_test_);
    DynamicVector<double> result(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      integrand.evaluate(*vector_test_, x, result);
      EXPECT_NEAR(std::sqrt(5.), result[0], 1e-13);
      EXPECT_NEAR(std::sqrt(std::pow(x[0] * x[1], 2) + std::pow(x[0] + x[1], 2)), result[1], 1e-13);
    }
  }

  // Edge case: evaluating at the origin (x=[0,0]) — basis function x[1] and x[0]*x[1]^3 both vanish
  void scalar_abs_at_origin_is_zero()
  {
    ScalarAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);

    const DomainType origin(0.);
    DynamicVector<double> result(2, 99.);
    integrand.evaluate(*scalar_test_, origin, result);
    // scalar_test_: phi_0(0,0)=0, phi_1(0,0)=0
    EXPECT_DOUBLE_EQ(0., result[0]);
    EXPECT_DOUBLE_EQ(0., result[1]);
  }

  // Edge case: result is non-negative even when basis values could theoretically be negative
  // The grid uses [0,3]x[0,1] so test basis {x[1], x[0]*x[1]^3} is always >= 0 there,
  // but abs.hh uses two_norm which is always non-negative regardless.
  void result_is_always_nonneg()
  {
    ScalarAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);

    const auto order = integrand.order(*scalar_test_);
    DynamicVector<double> result(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      integrand.evaluate(*scalar_test_, qp.position(), result);
      EXPECT_GE(result[0], 0.);
      EXPECT_GE(result[1], 0.);
    }
  }

  void copy_gives_same_results()
  {
    ScalarAbsIntegrand integrand;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    auto clone = integrand.copy_as_unary_element_integrand();
    clone->bind(element);

    const auto order = integrand.order(*scalar_test_);
    DynamicVector<double> result_orig(2, 0.), result_clone(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, x, result_orig);
      clone->evaluate(*scalar_test_, x, result_clone);
      for (size_t ii = 0; ii < 2; ++ii)
        EXPECT_DOUBLE_EQ(result_orig[ii], result_clone[ii]);
    }
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using BaseType::vector_ansatz_;
  using BaseType::vector_test_;
}; // struct AbsIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using AbsIntegrandTest = Dune::GDT::Test::AbsIntegrandTest<G>;
TYPED_TEST_SUITE(AbsIntegrandTest, Grids2D);

TYPED_TEST(AbsIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(AbsIntegrandTest, order_equals_basis_order)
{
  this->order_equals_basis_order();
}

TYPED_TEST(AbsIntegrandTest, scalar_abs_equals_absolute_value_of_basis)
{
  this->scalar_abs_equals_absolute_value_of_basis();
}

TYPED_TEST(AbsIntegrandTest, vector_abs_equals_two_norm_of_basis)
{
  this->vector_abs_equals_two_norm_of_basis();
}

TYPED_TEST(AbsIntegrandTest, scalar_abs_at_origin_is_zero)
{
  this->scalar_abs_at_origin_is_zero();
}

TYPED_TEST(AbsIntegrandTest, result_is_always_nonneg)
{
  this->result_is_always_nonneg();
}

TYPED_TEST(AbsIntegrandTest, copy_gives_same_results)
{
  this->copy_gives_same_results();
}
