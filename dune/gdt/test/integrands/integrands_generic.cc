// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/gdt/local/integrands/generic.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct GenericIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::LocalScalarBasisType;
  using typename BaseType::ScalarRangeType;

  using UnaryIntegrandType = GenericLocalUnaryElementIntegrand<E, 1>;
  using BinaryIntegrandType = GenericLocalBinaryElementIntegrand<E, 1>;

  void is_constructable() final
  {
    // Unary: order lambda returns constant 2, evaluate lambda sets result[ii] = ii+1
    [[maybe_unused]] UnaryIntegrandType unary_integrand([](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                                           const XT::Common::Parameter&) { return basis.order(); },
                                                        [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                                           const DomainType&,
                                                           DynamicVector<double>& result,
                                                           const XT::Common::Parameter&) {
                                                          for (size_t ii = 0; ii < basis.size(); ++ii)
                                                            result[ii] = static_cast<double>(ii + 1);
                                                        });

    // Unary with post-bind
    bool post_bind_called = false;
    [[maybe_unused]] UnaryIntegrandType unary_with_post_bind(
        [](const typename UnaryIntegrandType::LocalTestBasisType& basis, const XT::Common::Parameter&) {
          return basis.order();
        },
        [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
           const DomainType&,
           DynamicVector<double>& result,
           const XT::Common::Parameter&) {
          for (size_t ii = 0; ii < basis.size(); ++ii)
            result[ii] = 0.;
        },
        [&post_bind_called](const E&) { post_bind_called = true; });

    // Binary: order lambda, evaluate lambda
    [[maybe_unused]] BinaryIntegrandType binary_integrand(
        [](const typename BinaryIntegrandType::LocalTestBasisType& test,
           const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
           const XT::Common::Parameter&) { return test.order() + ansatz.order(); },
        [](const typename BinaryIntegrandType::LocalTestBasisType& test,
           const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
           const DomainType&,
           DynamicMatrix<double>& result,
           const XT::Common::Parameter&) {
          for (size_t ii = 0; ii < test.size(); ++ii)
            for (size_t jj = 0; jj < ansatz.size(); ++jj)
              result[ii][jj] = static_cast<double>((ii + 1) * (jj + 1));
        });
  }

  void order_is_forwarded_correctly()
  {
    const int expected_order = 42;
    UnaryIntegrandType integrand([expected_order](const typename UnaryIntegrandType::LocalTestBasisType&,
                                                  const XT::Common::Parameter&) { return expected_order; },
                                 [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                    const DomainType&,
                                    DynamicVector<double>& result,
                                    const XT::Common::Parameter&) {
                                   for (size_t ii = 0; ii < basis.size(); ++ii)
                                     result[ii] = 0.;
                                 });
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    EXPECT_EQ(expected_order, integrand.order(*scalar_test_));
  }

  void evaluate_lambda_is_called()
  {
    // Unary integrand that sets result[ii] = x[0] + x[1] for each basis function
    UnaryIntegrandType integrand([](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                    const XT::Common::Parameter&) { return basis.order(); },
                                 [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                    const DomainType& x,
                                    DynamicVector<double>& result,
                                    const XT::Common::Parameter&) {
                                   for (size_t ii = 0; ii < basis.size(); ++ii)
                                     result[ii] = x[0] + x[1];
                                 });
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    const auto integrand_order = integrand.order(*scalar_test_);
    DynamicVector<double> result(2, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), integrand_order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, x, result);
      EXPECT_DOUBLE_EQ(x[0] + x[1], result[0]);
      EXPECT_DOUBLE_EQ(x[0] + x[1], result[1]);
    }
  }

  void binary_evaluate_lambda_is_called()
  {
    // Binary integrand: result[ii][jj] = (ii+1) * (jj+1) * x[0]
    BinaryIntegrandType integrand([](const typename BinaryIntegrandType::LocalTestBasisType& test,
                                     const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
                                     const XT::Common::Parameter&) { return test.order() + ansatz.order(); },
                                  [](const typename BinaryIntegrandType::LocalTestBasisType& test,
                                     const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
                                     const DomainType& x,
                                     DynamicMatrix<double>& result,
                                     const XT::Common::Parameter&) {
                                    for (size_t ii = 0; ii < test.size(); ++ii)
                                      for (size_t jj = 0; jj < ansatz.size(); ++jj)
                                        result[ii][jj] = static_cast<double>((ii + 1) * (jj + 1)) * x[0];
                                  });
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<double> result(2, 2, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), integrand_order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      EXPECT_DOUBLE_EQ(1. * x[0], result[0][0]);
      EXPECT_DOUBLE_EQ(2. * x[0], result[0][1]);
      EXPECT_DOUBLE_EQ(2. * x[0], result[1][0]);
      EXPECT_DOUBLE_EQ(4. * x[0], result[1][1]);
    }
  }

  void post_bind_is_called()
  {
    bool was_called = false;
    UnaryIntegrandType integrand([](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                    const XT::Common::Parameter&) { return basis.order(); },
                                 [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                    const DomainType&,
                                    DynamicVector<double>& result,
                                    const XT::Common::Parameter&) {
                                   for (size_t ii = 0; ii < basis.size(); ++ii)
                                     result[ii] = 0.;
                                 },
                                 [&was_called](const E&) { was_called = true; });
    EXPECT_FALSE(was_called);
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    EXPECT_TRUE(was_called);
  }

  void copy_gives_same_results()
  {
    UnaryIntegrandType unary_integrand([](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                          const XT::Common::Parameter&) { return basis.order(); },
                                       [](const typename UnaryIntegrandType::LocalTestBasisType& basis,
                                          const DomainType& x,
                                          DynamicVector<double>& result,
                                          const XT::Common::Parameter&) {
                                         for (size_t ii = 0; ii < basis.size(); ++ii)
                                           result[ii] = x[0] * x[1];
                                       });
    this->check_unary_clone_matches(unary_integrand, *scalar_test_);

    BinaryIntegrandType binary_integrand([](const typename BinaryIntegrandType::LocalTestBasisType& test,
                                            const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
                                            const XT::Common::Parameter&) { return test.order() + ansatz.order(); },
                                         [](const typename BinaryIntegrandType::LocalTestBasisType& test,
                                            const typename BinaryIntegrandType::LocalAnsatzBasisType& ansatz,
                                            const DomainType& x,
                                            DynamicMatrix<double>& result,
                                            const XT::Common::Parameter&) {
                                           for (size_t ii = 0; ii < test.size(); ++ii)
                                             for (size_t jj = 0; jj < ansatz.size(); ++jj)
                                               result[ii][jj] = x[0] * x[1];
                                         });
    this->check_binary_clone_matches(binary_integrand);
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using BaseType::vector_ansatz_;
  using BaseType::vector_test_;
}; // struct GenericIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using GenericIntegrandTest = Dune::GDT::Test::GenericIntegrandTest<G>;
TYPED_TEST_SUITE(GenericIntegrandTest, Grids2D);

TYPED_TEST(GenericIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(GenericIntegrandTest, order_is_forwarded_correctly)
{
  this->order_is_forwarded_correctly();
}

TYPED_TEST(GenericIntegrandTest, evaluate_lambda_is_called)
{
  this->evaluate_lambda_is_called();
}

TYPED_TEST(GenericIntegrandTest, binary_evaluate_lambda_is_called)
{
  this->binary_evaluate_lambda_is_called();
}

TYPED_TEST(GenericIntegrandTest, post_bind_is_called)
{
  this->post_bind_is_called();
}

TYPED_TEST(GenericIntegrandTest, copy_gives_same_results)
{
  this->copy_gives_same_results();
}
