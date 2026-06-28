// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/geometry/quadraturerules.hh>

#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/functions/generic/grid-function.hh>

#include <dune/gdt/local/integrands/linear-advection.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct LinearAdvectionIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::MatrixType;
  using IntegrandType = LocalLinearAdvectionIntegrand<E>;

  void is_constructable() override final
  {
    // construction from a GenericGridFunction (element-local vector function)
    const XT::Functions::GenericGridFunction<E, d> velocity_gf(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    [[maybe_unused]] IntegrandType integrand_div(velocity_gf);
    [[maybe_unused]] IntegrandType integrand_div2(velocity_gf, /*advection_in_divergence_form=*/true);
    [[maybe_unused]] IntegrandType integrand_nodiv(velocity_gf, /*advection_in_divergence_form=*/false);

    // construction from a GenericFunction (static vector function)
    const XT::Functions::GenericFunction<d, d> velocity_func(
        0, [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    [[maybe_unused]] IntegrandType integrand_from_func(velocity_func);

    // copy constructor
    [[maybe_unused]] IntegrandType integrand_copy(integrand_div);

    // copy_as_binary_element_integrand
    auto cloned = integrand_div.copy_as_binary_element_integrand();
    EXPECT_NE(nullptr, cloned);
  }

  virtual void order_is_correct()
  {
    // For constant direction (order 0): order = 0 + test.order + ansatz.order = 0 + 4 + 3 = 7
    const XT::Functions::GenericGridFunction<E, d> const_vel(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    IntegrandType integrand_const(const_vel, true);
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand_const.bind(element);
    EXPECT_EQ(0 + 4 + 3, integrand_const.order(*scalar_test_, *scalar_ansatz_));

    // For linear direction (order 1): order = 1 + 4 + 3 = 8
    const XT::Functions::GenericGridFunction<E, d> linear_vel(
        1,
        [](const E&) {},
        [](const DomainType& x, const XT::Common::Parameter&) { return FieldVector<D, d>{{x[0], x[1]}}; });
    IntegrandType integrand_linear(linear_vel, true);
    integrand_linear.bind(element);
    EXPECT_EQ(1 + 4 + 3, integrand_linear.order(*scalar_test_, *scalar_ansatz_));

    // order is the same for divergence and non-divergence form
    IntegrandType integrand_nodiv(const_vel, false);
    integrand_nodiv.bind(element);
    EXPECT_EQ(0 + 4 + 3, integrand_nodiv.order(*scalar_test_, *scalar_ansatz_));
  }

  virtual void evaluates_correctly_divergence_form()
  {
    // velocity v = (1, 0) constant, divergence form
    // formula: result[i][j] = -ansatz_val[j] * (v . test_grad[i])
    //
    // scalar_test_  = {y, xy^3},  jacobians = {(0,1), (y^3, 3xy^2)}
    // scalar_ansatz_ = {x, x^2*y}, jacobians = {(1,0), (2xy, x^2)}
    //
    // v=(1,0): v . test_grad[0] = (1,0).(0,1) = 0
    //          v . test_grad[1] = (1,0).(y^3, 3xy^2) = y^3
    //
    // expected result[i][j] = -ansatz[j] * (v . test_grad[i]):
    //   [0][0] = -x * 0 = 0
    //   [0][1] = -x^2*y * 0 = 0
    //   [1][0] = -x * y^3
    //   [1][1] = -x^2*y * y^4  <- NOTE: -x^2*y * y^3 = -x^2*y^4, todo verify this is correct behavior
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    IntegrandType integrand(velocity, /*advection_in_divergence_form=*/true);
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> result(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      // row i = test function index, col j = ansatz function index
      EXPECT_NEAR(0., result[0][0], 1e-13);
      EXPECT_NEAR(0., result[0][1], 1e-13);
      EXPECT_NEAR(-x[0] * std::pow(x[1], 3), result[1][0], 1e-13);
      EXPECT_NEAR(-std::pow(x[0], 2) * x[1] * std::pow(x[1], 3), result[1][1], 1e-13);
    }
  }

  virtual void evaluates_correctly_non_divergence_form()
  {
    // velocity v = (1, 0) constant, non-divergence form
    // formula: result[i][j] = (v . ansatz_grad[j]) * test_val[i]
    //
    // scalar_ansatz_ = {x, x^2*y}, jacobians = {(1,0), (2xy, x^2)}
    // scalar_test_   = {y, xy^3}
    //
    // v=(1,0): v . ansatz_grad[0] = (1,0).(1,0) = 1
    //          v . ansatz_grad[1] = (1,0).(2xy, x^2) = 2xy
    //
    // expected result[i][j] = (v . ansatz_grad[j]) * test[i]:
    //   [0][0] = 1 * y = y
    //   [0][1] = 2xy * y = 2xy^2
    //   [1][0] = 1 * xy^3 = xy^3
    //   [1][1] = 2xy * xy^3 = 2x^2*y^4
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    IntegrandType integrand(velocity, /*advection_in_divergence_form=*/false);
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> result(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      EXPECT_NEAR(x[1], result[0][0], 1e-13);
      EXPECT_NEAR(2. * x[0] * std::pow(x[1], 2), result[0][1], 1e-13);
      EXPECT_NEAR(x[0] * std::pow(x[1], 3), result[1][0], 1e-13);
      EXPECT_NEAR(2. * std::pow(x[0], 2) * std::pow(x[1], 4), result[1][1], 1e-13);
    }
  }

  virtual void evaluates_correctly_with_variable_velocity()
  {
    // velocity v = (x, y) (order 1), tests both divergence and non-divergence forms
    //
    // Divergence form:
    //   v . test_grad[0] = (x,y).(0,1) = y
    //   v . test_grad[1] = (x,y).(y^3, 3xy^2) = x*y^3 + 3xy^3 = 4xy^3  <- TODO verify
    //   result[i][j] = -ansatz[j] * (v . test_grad[i]):
    //     [0][0] = -x * y
    //     [0][1] = -x^2*y * y = -x^2*y^2
    //     [1][0] = -x * 4xy^3 = -4x^2*y^3
    //     [1][1] = -x^2*y * 4xy^3 = -4x^3*y^4
    //
    // Non-divergence form:
    //   v . ansatz_grad[0] = (x,y).(1,0) = x
    //   v . ansatz_grad[1] = (x,y).(2xy, x^2) = 2x^2*y + x^2*y = 3x^2*y  <- TODO verify
    //   result[i][j] = (v . ansatz_grad[j]) * test[i]:
    //     [0][0] = x * y = xy
    //     [0][1] = 3x^2*y * y = 3x^2*y^2
    //     [1][0] = x * xy^3 = x^2*y^3
    //     [1][1] = 3x^2*y * xy^3 = 3x^3*y^4
    const XT::Functions::GenericGridFunction<E, d> velocity(
        1,
        [](const E&) {},
        [](const DomainType& x, const XT::Common::Parameter&) { return FieldVector<D, d>{{x[0], x[1]}}; });
    const auto element = *(grid_provider_->leaf_view().template begin<0>());

    {
      IntegrandType integrand(velocity, /*advection_in_divergence_form=*/true);
      integrand.bind(element);
      const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
      EXPECT_EQ(1 + 4 + 3, order);
      DynamicMatrix<D> result(2, 2, 0.);
      for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
        const auto& x = qp.position();
        integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
        // v.test_grad[1] = x*y^3 + y*(3*x*y^2) = xy^3 + 3xy^3 = 4xy^3
        EXPECT_NEAR(-x[0] * x[1], result[0][0], 1e-13);
        EXPECT_NEAR(-std::pow(x[0], 2) * x[1] * x[1], result[0][1], 1e-13);
        EXPECT_NEAR(-x[0] * 4. * x[0] * std::pow(x[1], 3), result[1][0], 1e-13);
        EXPECT_NEAR(-std::pow(x[0], 2) * x[1] * 4. * x[0] * std::pow(x[1], 3), result[1][1], 1e-13);
      }
    }

    {
      IntegrandType integrand(velocity, /*advection_in_divergence_form=*/false);
      integrand.bind(element);
      const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
      DynamicMatrix<D> result(2, 2, 0.);
      for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
        const auto& x = qp.position();
        integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
        // v.ansatz_grad[1] = x*(2xy) + y*(x^2) = 2x^2y + x^2y = 3x^2y
        EXPECT_NEAR(x[0] * x[1], result[0][0], 1e-13);
        EXPECT_NEAR(3. * std::pow(x[0], 2) * x[1] * x[1], result[0][1], 1e-13);
        EXPECT_NEAR(x[0] * x[0] * std::pow(x[1], 3), result[1][0], 1e-13);
        EXPECT_NEAR(3. * std::pow(x[0], 2) * x[1] * x[0] * std::pow(x[1], 3), result[1][1], 1e-13);
      }
    }
  }

  virtual void zero_velocity_gives_zero_result()
  {
    // edge case: zero velocity => all results must be zero
    const XT::Functions::GenericGridFunction<E, d> zero_vel(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{0., 0.}}; });
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    DynamicMatrix<D> result(2, 2, 0.);

    for (const bool div_form : {true, false}) {
      IntegrandType integrand(zero_vel, div_form);
      integrand.bind(element);
      const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
      for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
        integrand.evaluate(*scalar_test_, *scalar_ansatz_, qp.position(), result);
        for (size_t ii = 0; ii < 2; ++ii)
          for (size_t jj = 0; jj < 2; ++jj)
            EXPECT_DOUBLE_EQ(0., result[ii][jj]);
      }
    }
  }

  virtual void clone_gives_same_results()
  {
    // clone via copy constructor and copy_as_binary_element_integrand should produce identical results
    const XT::Functions::GenericGridFunction<E, d> velocity(
        1,
        [](const E&) {},
        [](const DomainType& x, const XT::Common::Parameter&) { return FieldVector<D, d>{{x[0], x[1]}}; });
    IntegrandType original(velocity, true);
    IntegrandType copy_ctor(original);
    auto cloned = original.copy_as_binary_element_integrand();

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    original.bind(element);
    copy_ctor.bind(element);
    cloned->bind(element);

    const auto order = original.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> res_orig(2, 2, 0.), res_copy(2, 2, 0.), res_clone(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d>::rule(element.type(), order)) {
      const auto& x = qp.position();
      original.evaluate(*scalar_test_, *scalar_ansatz_, x, res_orig);
      copy_ctor.evaluate(*scalar_test_, *scalar_ansatz_, x, res_copy);
      cloned->evaluate(*scalar_test_, *scalar_ansatz_, x, res_clone);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj) {
          EXPECT_DOUBLE_EQ(res_orig[ii][jj], res_copy[ii][jj]);
          EXPECT_DOUBLE_EQ(res_orig[ii][jj], res_clone[ii][jj]);
        }
    }
  }

  using BaseType::grid_provider_;
  using BaseType::is_simplex_grid_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using BaseType::vector_ansatz_;
  using BaseType::vector_test_;
}; // struct LinearAdvectionIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using LinearAdvectionIntegrandTest = Dune::GDT::Test::LinearAdvectionIntegrandTest<G>;
TYPED_TEST_SUITE(LinearAdvectionIntegrandTest, Grids2D);

TYPED_TEST(LinearAdvectionIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(LinearAdvectionIntegrandTest, order_is_correct)
{
  this->order_is_correct();
}

TYPED_TEST(LinearAdvectionIntegrandTest, evaluates_correctly_divergence_form)
{
  this->evaluates_correctly_divergence_form();
}

TYPED_TEST(LinearAdvectionIntegrandTest, evaluates_correctly_non_divergence_form)
{
  this->evaluates_correctly_non_divergence_form();
}

TYPED_TEST(LinearAdvectionIntegrandTest, evaluates_correctly_with_variable_velocity)
{
  this->evaluates_correctly_with_variable_velocity();
}

TYPED_TEST(LinearAdvectionIntegrandTest, zero_velocity_gives_zero_result)
{
  this->zero_velocity_gives_zero_result();
}

TYPED_TEST(LinearAdvectionIntegrandTest, clone_gives_same_results)
{
  this->clone_gives_same_results();
}
