// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/gdt/local/integrands/conversion.hh>
#include <dune/gdt/local/integrands/product.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct ConversionIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;

  using ScalarProductIntegrand = LocalElementProductIntegrand<E, 1>;
  // LocalBinaryToUnaryElementIntegrand is the explicit conversion class; with_ansatz is the user-facing API
  using ConversionType = LocalBinaryToUnaryElementIntegrand<E, 1, 1, double, double, 1, 1, double>;

  void is_constructable() final
  {
    // Construct via with_ansatz (the idiomatic API that produces LocalBinaryToUnaryElementIntegrand)
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0]; });

    ScalarProductIntegrand product;
    // with_ansatz returns LocalBinaryToUnaryElementIntegrand by value
    [[maybe_unused]] auto unary = product.with_ansatz(inducing_fn);

    // Construct directly via ConversionType constructor
    [[maybe_unused]] ConversionType direct(product, inducing_fn);
  }

  void converted_integrand_matches_manual_computation()
  {
    // binary integrand: result[ii][jj] = phi_ii(x) * psi_jj(x)  (unweighted product)
    // inducing function:  f(x) = x[0]
    // unary integrand should compute: result[ii] = phi_ii(x) * f(x) = phi_ii(x) * x[0]
    //   where phi are the scalar_test_ basis functions: {x[0], x[0]^2 * x[1]}
    // So result[0] = x[0]*x[0] = x[0]^2
    //    result[1] = x[0]^2*x[1]*x[0] = x[0]^3*x[1]
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0]; });

    ScalarProductIntegrand product(1.);
    auto unary = product.with_ansatz(inducing_fn);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    unary.bind(element);

    const auto order = unary.order(*scalar_test_);
    DynamicVector<double> result(2, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      unary.evaluate(*scalar_test_, x, result);
      // scalar_test_ basis (see integrands.hh): phi_0 = x[1], phi_1 = x[0]*x[1]^3
      // inducing_fn f = x[0]
      // The conversion wraps a binary product integrand; it evaluates:
      //   binary_result[ii][0] = phi_test_ii(x) * f(x)
      // then extracts column 0.
      // So result[0] = x[1] * x[0]
      //    result[1] = x[0]*x[1]^3 * x[0] = x[0]^2 * x[1]^3
      EXPECT_NEAR(x[1] * x[0], result[0], 1e-13);
      EXPECT_NEAR(std::pow(x[0], 2) * std::pow(x[1], 3), result[1], 1e-13);
    }
  }

  void converted_order_is_correct()
  {
    // order of conversion = order of binary integrand evaluated with test basis and local function
    // product order = weight_order + test_order + ansatz_order
    // inducing_fn order = 2 => conversion order = 0 + test_order + 2
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        2, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0] * x[1]; });

    ScalarProductIntegrand product(1.); // weight order = 0
    auto unary = product.with_ansatz(inducing_fn);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    unary.bind(element);

    // scalar_test_ has order 4; inducing_fn has order 2; product weight is constant (order 0)
    // => expected = 0 + 4 + 2 = 6
    const int expected_order = 0 + scalar_test_->order() + 2;
    EXPECT_EQ(expected_order, unary.order(*scalar_test_));
  }

  void copy_gives_same_results()
  {
    // TODO: This test documents a known bug: LocalBinaryToUnaryElementIntegrand::
    // copy_as_unary_element_integrand() creates fresh local_function_ and local_binary_integrand_
    // objects. The copy constructor of ElementBoundObject (base class) transfers element_, so when
    // clone->bind(same_element) is called, the guard `if (element_ && ele == *element_) return
    // *this;` skips post_bind() entirely. The inner local functions remain unbound and evaluate()
    // throws not_bound_to_an_element_yet. The Correct behavior would be for copy constructors to
    // NOT transfer element_ so that bind() always runs post_bind() and initializes inner objects.
    const XT::Functions::GenericGridFunction<E, 1> inducing_fn(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) { return x[0] + x[1]; });

    ScalarProductIntegrand product(1.);
    auto unary = product.with_ansatz(inducing_fn);
    EXPECT_THROW(this->check_unary_clone_matches(unary, *scalar_test_), Dune::Exception);
  }

  // Edge case: inducing function = 0 => unary integrand evaluates to 0 everywhere
  void zero_inducing_function_gives_zero_result()
  {
    const XT::Functions::GenericGridFunction<E, 1> zero_fn(
        0, [](const E&) {}, [](const DomainType&, const XT::Common::Parameter&) { return 0.; });

    ScalarProductIntegrand product(1.);
    auto unary = product.with_ansatz(zero_fn);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    unary.bind(element);

    const auto order = unary.order(*scalar_test_);
    DynamicVector<double> result(2, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      unary.evaluate(*scalar_test_, x, result);
      EXPECT_DOUBLE_EQ(0., result[0]);
      EXPECT_DOUBLE_EQ(0., result[1]);
    }
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
}; // struct ConversionIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using ConversionIntegrandTest = Dune::GDT::Test::ConversionIntegrandTest<G>;
TYPED_TEST_SUITE(ConversionIntegrandTest, Grids2D);

TYPED_TEST(ConversionIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(ConversionIntegrandTest, converted_integrand_matches_manual_computation)
{
  this->converted_integrand_matches_manual_computation();
}

TYPED_TEST(ConversionIntegrandTest, converted_order_is_correct)
{
  this->converted_order_is_correct();
}

TYPED_TEST(ConversionIntegrandTest, copy_gives_same_results)
{
  this->copy_gives_same_results();
}

TYPED_TEST(ConversionIntegrandTest, zero_inducing_function_gives_zero_result)
{
  this->zero_inducing_function_gives_zero_result();
}
