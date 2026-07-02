// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/gdt/local/integrands/combined.hh>
#include <dune/gdt/local/integrands/product.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct CombinedIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::MatrixType;

  using ScalarProductIntegrand = LocalElementProductIntegrand<E, 1>;
  using BinarySum = LocalBinaryElementIntegrandSum<E, 1, 1, double, double, 1, 1, double>;
  using UnarySum = LocalUnaryElementIntegrandSum<E, 1, 1, double, double>;

  void is_constructable() final
  {
    // Build sum of two product integrands with scalar weights
    ScalarProductIntegrand left(2.);
    ScalarProductIntegrand right(3.);
    // Binary sum: via constructor and via operator+
    [[maybe_unused]] BinarySum binary_sum_via_ctor(left, right);
    [[maybe_unused]] auto binary_sum_via_op = left + right;
    // Unary sum: via constructor and via operator+
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0]; });
    auto unary_left = left.with_ansatz(inducing_fn);
    auto unary_right = right.with_ansatz(inducing_fn);
    [[maybe_unused]] UnarySum unary_sum_via_ctor(unary_left, unary_right);
    [[maybe_unused]] auto unary_sum_via_op = unary_left + unary_right;
  }

  void binary_sum_order_is_max_of_parts()
  {
    // Product with weight x*y (order 2) and constant weight 1.0 (order 0)
    const XT::Functions::GenericGridFunction<E, 1> weight_fn(
        2, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0] * x[1]; });
    ScalarProductIntegrand left(weight_fn); // order = 2 + test + ansatz
    ScalarProductIntegrand right(1.); // order = 0 + test + ansatz

    auto sum = left + right;
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    sum.bind(element);
    left.bind(element);
    right.bind(element);

    const int order_left = left.order(*scalar_test_, *scalar_ansatz_);
    const int order_right = right.order(*scalar_test_, *scalar_ansatz_);
    const int order_sum = sum.order(*scalar_test_, *scalar_ansatz_);
    EXPECT_EQ(std::max(order_left, order_right), order_sum);
  }

  void binary_sum_evaluates_as_elementwise_sum()
  {
    // sum of weight_a and weight_b: a=2, b=3 => sum result = (2+3)*phi*psi = 5*phi*psi
    ScalarProductIntegrand left(2.);
    ScalarProductIntegrand right(3.);
    auto sum = left + right;

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    left.bind(element);
    right.bind(element);
    sum.bind(element);

    const auto order = sum.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<double> result_left(2, 2, 0.);
    DynamicMatrix<double> result_right(2, 2, 0.);
    DynamicMatrix<double> result_sum(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      left.evaluate(*scalar_test_, *scalar_ansatz_, x, result_left);
      right.evaluate(*scalar_test_, *scalar_ansatz_, x, result_right);
      sum.evaluate(*scalar_test_, *scalar_ansatz_, x, result_sum);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_NEAR(result_left[ii][jj] + result_right[ii][jj], result_sum[ii][jj], 1e-14);
    }
  }

  void binary_sum_copy_gives_same_results()
  {
    ScalarProductIntegrand left(2.);
    ScalarProductIntegrand right(3.);
    auto sum = left + right;

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    sum.bind(element);
    auto clone = sum.copy_as_binary_element_integrand();
    clone->bind(element);

    const auto order = sum.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<double> result_orig(2, 2, 0.);
    DynamicMatrix<double> result_clone(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      sum.evaluate(*scalar_test_, *scalar_ansatz_, x, result_orig);
      clone->evaluate(*scalar_test_, *scalar_ansatz_, x, result_clone);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_DOUBLE_EQ(result_orig[ii][jj], result_clone[ii][jj]);
    }
  }

  void unary_sum_evaluates_as_elementwise_sum()
  {
    // Use LocalBinaryToUnaryElementIntegrand to get unary integrands, then sum them.
    // For simplicity, use product.with_ansatz (if available) or test via conversion.hh approach.
    // Here we test LocalUnaryElementIntegrandSum directly by using the interface operator+.
    // We need unary integrands — use LocalElementAbsIntegrand from abs.hh for that.
    // But to keep this test self-contained with only combined.hh + product.hh, we use
    // the with_ansatz mechanism provided by LocalBinaryElementIntegrandInterface.
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn_a(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0]; });
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn_b(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[1]; });

    ScalarProductIntegrand product_a(1.);
    ScalarProductIntegrand product_b(1.);

    // LocalBinaryElementIntegrandInterface provides with_ansatz which creates a unary integrand (by value)
    auto unary_a = product_a.with_ansatz(inducing_fn_a);
    auto unary_b = product_b.with_ansatz(inducing_fn_b);

    // sum via operator+
    auto sum = unary_a + unary_b;

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    unary_a.bind(element);
    unary_b.bind(element);
    sum.bind(element);

    const auto order = sum.order(*scalar_test_);
    DynamicVector<double> result_a(2, 0.), result_b(2, 0.), result_sum(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      unary_a.evaluate(*scalar_test_, x, result_a);
      unary_b.evaluate(*scalar_test_, x, result_b);
      sum.evaluate(*scalar_test_, x, result_sum);
      for (size_t ii = 0; ii < 2; ++ii)
        EXPECT_NEAR(result_a[ii] + result_b[ii], result_sum[ii], 1e-14);
    }
  }

  void unary_sum_copy_gives_same_results()
  {
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0]; });

    ScalarProductIntegrand product_a(2.);
    ScalarProductIntegrand product_b(3.);
    auto unary_a = product_a.with_ansatz(inducing_fn);
    auto unary_b = product_b.with_ansatz(inducing_fn);
    auto sum = unary_a + unary_b;

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    sum.bind(element);
    auto clone = sum.copy_as_unary_element_integrand();
    clone->bind(element);

    const auto order = sum.order(*scalar_test_);
    DynamicVector<double> result_orig(2, 0.), result_clone(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      sum.evaluate(*scalar_test_, x, result_orig);
      clone->evaluate(*scalar_test_, x, result_clone);
      for (size_t ii = 0; ii < 2; ++ii)
        EXPECT_DOUBLE_EQ(result_orig[ii], result_clone[ii]);
    }
  }

  // Edge case: sum of two integrands with same weight — result should be 2x single
  void binary_sum_with_equal_weights_is_double()
  {
    ScalarProductIntegrand left(1.);
    ScalarProductIntegrand right(1.);
    ScalarProductIntegrand single(2.);
    auto sum = left + right;

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    sum.bind(element);
    single.bind(element);

    const auto order = sum.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<double> result_sum(2, 2, 0.);
    DynamicMatrix<double> result_single(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      sum.evaluate(*scalar_test_, *scalar_ansatz_, x, result_sum);
      single.evaluate(*scalar_test_, *scalar_ansatz_, x, result_single);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_NEAR(result_single[ii][jj], result_sum[ii][jj], 1e-14);
    }
  }

  using BaseType::grid_provider_;
  using BaseType::is_simplex_grid_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using BaseType::vector_ansatz_;
  using BaseType::vector_test_;
}; // struct CombinedIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using CombinedIntegrandTest = Dune::GDT::Test::CombinedIntegrandTest<G>;
TYPED_TEST_SUITE(CombinedIntegrandTest, Grids2D);

TYPED_TEST(CombinedIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(CombinedIntegrandTest, binary_sum_order_is_max_of_parts)
{
  this->binary_sum_order_is_max_of_parts();
}

TYPED_TEST(CombinedIntegrandTest, binary_sum_evaluates_as_elementwise_sum)
{
  this->binary_sum_evaluates_as_elementwise_sum();
}

TYPED_TEST(CombinedIntegrandTest, binary_sum_copy_gives_same_results)
{
  this->binary_sum_copy_gives_same_results();
}

TYPED_TEST(CombinedIntegrandTest, unary_sum_evaluates_as_elementwise_sum)
{
  this->unary_sum_evaluates_as_elementwise_sum();
}

TYPED_TEST(CombinedIntegrandTest, unary_sum_copy_gives_same_results)
{
  this->unary_sum_copy_gives_same_results();
}

TYPED_TEST(CombinedIntegrandTest, binary_sum_with_equal_weights_is_double)
{
  this->binary_sum_with_equal_weights_is_double();
}
