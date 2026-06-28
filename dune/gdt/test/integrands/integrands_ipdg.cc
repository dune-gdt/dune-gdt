// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/functions/generic/grid-function.hh>
#include <dune/xt/grid/intersection.hh>

#include <dune/gdt/local/integrands/ipdg.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct IPDGIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;
  using typename BaseType::ScalarRangeType;
  using typename BaseType::VectorJacobianType;

  using InnerPenaltyType = LocalIPDGIntegrands::InnerPenalty<I>;
  using BoundaryPenaltyType = LocalIPDGIntegrands::BoundaryPenalty<I>;

  void is_constructable() final
  {
    // InnerPenalty: penalty only (weight = identity by default)
    [[maybe_unused]] InnerPenaltyType inner1(1.0);
    // InnerPenalty: penalty + constant weight
    [[maybe_unused]] InnerPenaltyType inner2(2.0, 1.0);
    // InnerPenalty: penalty + matrix weight function
    const XT::Functions::GenericGridFunction<E, d, d> weight_gf(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return VectorJacobianType{{2., 0.}, {0., 2.}}; });
    [[maybe_unused]] InnerPenaltyType inner3(3.0, weight_gf);
    // InnerPenalty: with custom intersection diameter
    [[maybe_unused]] InnerPenaltyType inner4(1.0, 1.0, [](const I& i) { return XT::Grid::diameter(i); });
    // Copy constructor
    InnerPenaltyType inner_src(4.0);
    [[maybe_unused]] InnerPenaltyType inner5(inner_src);
    // copy_as_quaternary_intersection_integrand
    auto clone = inner_src.copy_as_quaternary_intersection_integrand();
    EXPECT_NE(clone, nullptr);

    // BoundaryPenalty: penalty only
    [[maybe_unused]] BoundaryPenaltyType bnd1(1.0);
    // BoundaryPenalty: penalty + constant weight
    [[maybe_unused]] BoundaryPenaltyType bnd2(2.0, 1.0);
    // BoundaryPenalty: penalty + matrix weight function
    [[maybe_unused]] BoundaryPenaltyType bnd3(3.0, weight_gf);
    // BoundaryPenalty: with custom intersection diameter
    [[maybe_unused]] BoundaryPenaltyType bnd4(1.0, 1.0, [](const I& i) { return XT::Grid::diameter(i); });
    // Copy constructor
    BoundaryPenaltyType bnd_src(5.0);
    [[maybe_unused]] BoundaryPenaltyType bnd5(bnd_src);
    // copy_as_binary_intersection_integrand
    auto bclone = bnd_src.copy_as_binary_intersection_integrand();
    EXPECT_NE(bclone, nullptr);
    // inside() returns true for BoundaryPenalty
    EXPECT_TRUE(bnd1.inside());
  }

  void inner_penalty_post_bind_throws_on_boundary_intersection()
  {
    InnerPenaltyType integrand(1.0);
    const auto& bnd = find_boundary_intersection();
    EXPECT_THROW(integrand.bind(bnd), Dune::Exception);
  }

  void inner_penalty_order_is_correct()
  {
    // order = max(weight_order_in, weight_order_out)
    //       + max(test_order_in, test_order_out)
    //       + max(ansatz_order_in, ansatz_order_out)
    // With unit weight (order 0), scalar_test_ (order 4), scalar_ansatz_ (order 3):
    // expected order = 0 + 4 + 3 = 7
    InnerPenaltyType integrand(10.0);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    EXPECT_EQ(7, integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_));
  }

  void inner_penalty_evaluates_correctly_unit_weight()
  {
    // Formula with weight = I (identity):
    //   delta_plus = delta_minus = n · n = 1
    //   weight_harmonic = (1 * 1)/(1 + 1) = 0.5
    //   h = diameter(intersection)
    //   effective_penalty = sigma * 0.5 / h
    //   result_in_in[i][j]  = effective_penalty * phi_j_in * psi_i_in
    //   result_in_out[i][j] = -effective_penalty * phi_j_out * psi_i_in
    //   result_out_in[i][j] = -effective_penalty * phi_j_in * psi_i_out
    //   result_out_out[i][j] = effective_penalty * phi_j_out * psi_i_out
    const double sigma = 8.0;
    InnerPenaltyType integrand(sigma);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const double h = XT::Grid::diameter(intersection);
    const double effective_penalty = sigma * 0.5 / h;
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(
          *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, x, rin_in, rin_out, rout_in, rout_out);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto x_out = intersection.geometryInOutside().global(x);
      std::vector<ScalarRangeType> phi_in, phi_out, psi_in, psi_out;
      scalar_ansatz_->evaluate(x_in, phi_in);
      scalar_ansatz_->evaluate(x_out, phi_out);
      scalar_test_->evaluate(x_in, psi_in);
      scalar_test_->evaluate(x_out, psi_out);
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          EXPECT_NEAR(effective_penalty * phi_in[jj][0] * psi_in[ii][0], rin_in[ii][jj], 1e-13);
          EXPECT_NEAR(-effective_penalty * phi_out[jj][0] * psi_in[ii][0], rin_out[ii][jj], 1e-13);
          EXPECT_NEAR(-effective_penalty * phi_in[jj][0] * psi_out[ii][0], rout_in[ii][jj], 1e-13);
          EXPECT_NEAR(effective_penalty * phi_out[jj][0] * psi_out[ii][0], rout_out[ii][jj], 1e-13);
        }
      }
    }
  }

  void inner_penalty_evaluates_correctly_with_constant_weight()
  {
    // With weight = c * I (c > 0, constant scalar * identity):
    //   delta_plus = delta_minus = c * n · n = c
    //   weight_harmonic = (c * c) / (c + c) = c/2
    //   effective_penalty = sigma * c/2 / h
    // So effective_penalty changes proportionally to c.
    // We verify that using weight = 2 * I gives effective_penalty = sigma * 1 / h
    // (i.e., weight harmonic mean doubles from 0.5 to 1).
    const double sigma = 8.0;
    const double c = 2.0;
    // Create 2*I weight function
    const XT::Functions::GenericGridFunction<E, d, d> weight_2I(
        0,
        [](const E&) {},
        [c](const DomainType&, const XT::Common::Parameter&) {
          VectorJacobianType mat{{0., 0.}, {0., 0.}};
          for (size_t i = 0; i < d; ++i)
            mat[i][i] = c;
          return mat;
        });
    InnerPenaltyType integrand(sigma, weight_2I);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const double h = XT::Grid::diameter(intersection);
    // delta = c * (n · I · n) = c * |n|^2 = c  (unit normal)
    // weight_harmonic = (c * c) / (c + c) = c/2
    const double effective_penalty = sigma * (c / 2.) / h;
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(
          *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, x, rin_in, rin_out, rout_in, rout_out);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto x_out = intersection.geometryInOutside().global(x);
      std::vector<ScalarRangeType> phi_in, phi_out, psi_in, psi_out;
      scalar_ansatz_->evaluate(x_in, phi_in);
      scalar_ansatz_->evaluate(x_out, phi_out);
      scalar_test_->evaluate(x_in, psi_in);
      scalar_test_->evaluate(x_out, psi_out);
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          EXPECT_NEAR(effective_penalty * phi_in[jj][0] * psi_in[ii][0], rin_in[ii][jj], 1e-12);
          EXPECT_NEAR(-effective_penalty * phi_out[jj][0] * psi_in[ii][0], rin_out[ii][jj], 1e-12);
          EXPECT_NEAR(-effective_penalty * phi_in[jj][0] * psi_out[ii][0], rout_in[ii][jj], 1e-12);
          EXPECT_NEAR(effective_penalty * phi_out[jj][0] * psi_out[ii][0], rout_out[ii][jj], 1e-12);
        }
      }
    }
  }

  void inner_penalty_zero_penalty_gives_zero_result()
  {
    // TODO: Is zero penalty actually valid? The code divides sigma*weight/h, so sigma=0 → 0 results.
    InnerPenaltyType integrand(0.0);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    const auto center = FieldVector<D, d - 1>(0.5);
    integrand.evaluate(
        *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, center, rin_in, rin_out, rout_in, rout_out);
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj) {
        EXPECT_NEAR(0., rin_in[ii][jj], 1e-15);
        EXPECT_NEAR(0., rin_out[ii][jj], 1e-15);
        EXPECT_NEAR(0., rout_in[ii][jj], 1e-15);
        EXPECT_NEAR(0., rout_out[ii][jj], 1e-15);
      }
  }

  void boundary_penalty_order_is_correct()
  {
    // order = weight_order + test_order + ansatz_order = 0 + 4 + 3 = 7
    BoundaryPenaltyType integrand(5.0);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    EXPECT_EQ(7, integrand.order(*scalar_test_, *scalar_ansatz_));
  }

  void boundary_penalty_evaluates_correctly()
  {
    // Formula:
    //   n_dot_weight_n = n · (I * n) = 1  (unit weight, unit normal)
    //   h = diameter(intersection)
    //   effective_penalty = sigma * 1 / h
    //   result[i][j] = effective_penalty * phi_j * psi_i
    const double sigma = 5.0;
    BoundaryPenaltyType integrand(sigma);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const double h = XT::Grid::diameter(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> result(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto n = intersection.unitOuterNormal(x);
      // With unit weight (I): n · (I * n) = n · n = 1
      const double n_dot_weight_n = n * n; // should be 1 for unit normal
      const double effective_penalty = sigma * n_dot_weight_n / h;
      std::vector<ScalarRangeType> phi, psi;
      scalar_ansatz_->evaluate(x_in, phi);
      scalar_test_->evaluate(x_in, psi);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_NEAR(effective_penalty * phi[jj][0] * psi[ii][0], result[ii][jj], 1e-13);
    }
  }

  void boundary_penalty_zero_penalty_gives_zero_result()
  {
    BoundaryPenaltyType integrand(0.0);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto center = FieldVector<D, d - 1>(0.5);
    DynamicMatrix<D> result(2, 2, 0.);
    integrand.evaluate(*scalar_test_, *scalar_ansatz_, center, result);
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_NEAR(0., result[ii][jj], 1e-15);
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
}; // struct IPDGIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using IPDGIntegrandTest = Dune::GDT::Test::IPDGIntegrandTest<G>;
TYPED_TEST_SUITE(IPDGIntegrandTest, Grids2D);

TYPED_TEST(IPDGIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(IPDGIntegrandTest, inner_penalty_post_bind_throws_on_boundary_intersection)
{
  this->inner_penalty_post_bind_throws_on_boundary_intersection();
}

TYPED_TEST(IPDGIntegrandTest, inner_penalty_order_is_correct)
{
  this->inner_penalty_order_is_correct();
}

TYPED_TEST(IPDGIntegrandTest, inner_penalty_evaluates_correctly_unit_weight)
{
  this->inner_penalty_evaluates_correctly_unit_weight();
}

TYPED_TEST(IPDGIntegrandTest, inner_penalty_evaluates_correctly_with_constant_weight)
{
  this->inner_penalty_evaluates_correctly_with_constant_weight();
}

TYPED_TEST(IPDGIntegrandTest, inner_penalty_zero_penalty_gives_zero_result)
{
  this->inner_penalty_zero_penalty_gives_zero_result();
}

TYPED_TEST(IPDGIntegrandTest, boundary_penalty_order_is_correct)
{
  this->boundary_penalty_order_is_correct();
}

TYPED_TEST(IPDGIntegrandTest, boundary_penalty_evaluates_correctly)
{
  this->boundary_penalty_evaluates_correctly();
}

TYPED_TEST(IPDGIntegrandTest, boundary_penalty_zero_penalty_gives_zero_result)
{
  this->boundary_penalty_zero_penalty_gives_zero_result();
}
