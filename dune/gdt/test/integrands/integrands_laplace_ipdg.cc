// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/functions/generic/grid-function.hh>
#include <dune/xt/grid/intersection.hh>

#include <dune/gdt/local/integrands/laplace-ipdg.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct LaplaceIPDGIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;
  using typename BaseType::ScalarJacobianType;
  using typename BaseType::ScalarRangeType;
  using typename BaseType::VectorJacobianType;

  using InnerCouplingType = LocalLaplaceIPDGIntegrands::InnerCoupling<I>;
  using DirichletCouplingType = LocalLaplaceIPDGIntegrands::DirichletCoupling<I>;
  using UnaryBaseType = LocalUnaryIntersectionIntegrandInterface<I>;
  using BinaryBaseType = LocalBinaryIntersectionIntegrandInterface<I>;

  I find_inner_intersection() const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : Dune::elements(gv))
      for (const auto& intersection : Dune::intersections(gv, element))
        if (intersection.neighbor())
          return intersection;
    DUNE_THROW(Dune::InvalidStateException, "No inner intersection found in grid!");
  }

  I find_boundary_intersection() const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : Dune::elements(gv))
      for (const auto& intersection : Dune::intersections(gv, element))
        if (!intersection.neighbor())
          return intersection;
    DUNE_THROW(Dune::InvalidStateException, "No boundary intersection found in grid!");
  }

  void is_constructable() final
  {
    // InnerCoupling: (symmetry_prefactor, diffusion)
    [[maybe_unused]] InnerCouplingType inner_nipdg(-1., 1.); // NIPDG
    [[maybe_unused]] InnerCouplingType inner_iipdg(0., 1.); // IIPDG
    [[maybe_unused]] InnerCouplingType inner_sipdg(1., 1.); // SIPDG
    // InnerCoupling: with custom diffusion and weight
    const XT::Functions::GenericGridFunction<E, d, d> diffusion(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return VectorJacobianType{{2., 0.}, {0., 2.}}; });
    [[maybe_unused]] InnerCouplingType inner_with_diffusion(1., diffusion);
    [[maybe_unused]] InnerCouplingType inner_with_weight(1., diffusion, diffusion);
    // InnerCoupling: copy constructor
    InnerCouplingType src(1., 1.);
    [[maybe_unused]] InnerCouplingType inner_copy(src);
    // copy_as_quaternary_intersection_integrand
    auto clone = src.copy_as_quaternary_intersection_integrand();
    EXPECT_NE(clone, nullptr);

    // DirichletCoupling: (symmetry_prefactor, diffusion)
    [[maybe_unused]] DirichletCouplingType dir1(1., 1.);
    [[maybe_unused]] DirichletCouplingType dir2(-1., 1.);
    // DirichletCoupling: with dirichlet_data
    [[maybe_unused]] DirichletCouplingType dir3(1., 1., 0.5);
    [[maybe_unused]] DirichletCouplingType dir4(0., diffusion, 1.0);
    // DirichletCoupling: copy constructor
    DirichletCouplingType dir_src(1., 1.);
    [[maybe_unused]] DirichletCouplingType dir_copy(dir_src);
    // copy_as_binary / copy_as_unary
    auto dir_binary_clone = dir_src.copy_as_binary_intersection_integrand();
    EXPECT_NE(dir_binary_clone, nullptr);
    auto dir_unary_clone = dir_src.copy_as_unary_intersection_integrand();
    EXPECT_NE(dir_unary_clone, nullptr);
    // inside() returns true for both uses
    EXPECT_TRUE(dir_src.inside());
  }

  void inner_coupling_post_bind_throws_on_boundary_intersection()
  {
    InnerCouplingType integrand(1., 1.);
    const auto& bnd = find_boundary_intersection();
    EXPECT_THROW(integrand.bind(bnd), Dune::Exception);
  }

  // Helper that verifies InnerCoupling::evaluate for a given symmetry_prefactor.
  //
  // Formula with diffusion = identity and weight = identity:
  //   delta_plus = delta_minus = n · n = 1
  //   weight_minus = delta_plus / (delta_plus + delta_minus) = 0.5
  //   weight_plus  = delta_minus / (delta_plus + delta_minus) = 0.5
  //
  //   result_in_in[i][j]   = -0.5 * (grad_phi_in[j] · n) * psi_in[i]
  //                          - s * 0.5 * phi_in[j] * (grad_psi_in[i] · n)
  //   result_in_out[i][j]  = -0.5 * (grad_phi_out[j] · n) * psi_in[i]
  //                          + s * 0.5 * phi_out[j] * (grad_psi_in[i] · n)
  //   result_out_in[i][j]  = +0.5 * (grad_phi_in[j] · n) * psi_out[i]
  //                          - s * 0.5 * phi_in[j] * (grad_psi_out[i] · n)
  //   result_out_out[i][j] = +0.5 * (grad_phi_out[j] · n) * psi_out[i]
  //                          + s * 0.5 * phi_out[j] * (grad_psi_out[i] · n)
  void check_inner_coupling(const double symmetry_prefactor)
  {
    InnerCouplingType integrand(symmetry_prefactor, 1. /*diffusion=I*/, 1. /*weight=I*/);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(
          *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, x, rin_in, rin_out, rout_in, rout_out);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto x_out = intersection.geometryInOutside().global(x);
      const auto n = intersection.unitOuterNormal(x);
      // Evaluate basis functions and their gradients independently.
      std::vector<ScalarRangeType> phi_in, phi_out, psi_in, psi_out;
      std::vector<ScalarJacobianType> grad_phi_in, grad_phi_out, grad_psi_in, grad_psi_out;
      scalar_ansatz_->evaluate(x_in, phi_in);
      scalar_ansatz_->evaluate(x_out, phi_out);
      scalar_test_->evaluate(x_in, psi_in);
      scalar_test_->evaluate(x_out, psi_out);
      scalar_ansatz_->jacobians(x_in, grad_phi_in);
      scalar_ansatz_->jacobians(x_out, grad_phi_out);
      scalar_test_->jacobians(x_in, grad_psi_in);
      scalar_test_->jacobians(x_out, grad_psi_out);
      // With diffusion = I and weight = I:
      //   delta_plus = delta_minus = n · n = 1  →  weight_minus = weight_plus = 0.5
      const double wm = 0.5; // weight_minus
      const double wp = 0.5; // weight_plus
      const double s = symmetry_prefactor;
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          // grad[jj][0] is the gradient row (FieldVector<D,d>) for the scalar function.
          // With diffusion = I: (I * grad) * n = grad · n
          const double dphi_in_dot_n = grad_phi_in[jj][0] * n;
          const double dphi_out_dot_n = grad_phi_out[jj][0] * n;
          const double dpsi_in_dot_n = grad_psi_in[ii][0] * n;
          const double dpsi_out_dot_n = grad_psi_out[ii][0] * n;
          const double expected_in_in = -wm * dphi_in_dot_n * psi_in[ii][0] - s * wm * phi_in[jj][0] * dpsi_in_dot_n;
          const double expected_in_out = -wp * dphi_out_dot_n * psi_in[ii][0] + s * wm * phi_out[jj][0] * dpsi_in_dot_n;
          const double expected_out_in = wm * dphi_in_dot_n * psi_out[ii][0] - s * wp * phi_in[jj][0] * dpsi_out_dot_n;
          const double expected_out_out =
              wp * dphi_out_dot_n * psi_out[ii][0] + s * wp * phi_out[jj][0] * dpsi_out_dot_n;
          EXPECT_NEAR(expected_in_in, rin_in[ii][jj], 1e-13);
          EXPECT_NEAR(expected_in_out, rin_out[ii][jj], 1e-13);
          EXPECT_NEAR(expected_out_in, rout_in[ii][jj], 1e-13);
          EXPECT_NEAR(expected_out_out, rout_out[ii][jj], 1e-13);
        }
      }
      // For IIPDG (s=0), the symmetry term vanishes → only consistency terms remain.
      if (symmetry_prefactor == 0.) {
        for (size_t ii = 0; ii < 2; ++ii)
          for (size_t jj = 0; jj < 2; ++jj) {
            const double dphi_in_dot_n = grad_phi_in[jj][0] * n;
            const double dphi_out_dot_n = grad_phi_out[jj][0] * n;
            EXPECT_NEAR(-wm * dphi_in_dot_n * psi_in[ii][0], rin_in[ii][jj], 1e-13);
            EXPECT_NEAR(-wp * dphi_out_dot_n * psi_in[ii][0], rin_out[ii][jj], 1e-13);
          }
      }
    }
  }

  void inner_coupling_evaluates_correctly_nipdg()
  {
    check_inner_coupling(-1.0);
  }

  void inner_coupling_evaluates_correctly_iipdg()
  {
    check_inner_coupling(0.0);
  }

  void inner_coupling_evaluates_correctly_sipdg()
  {
    check_inner_coupling(1.0);
  }

  void inner_coupling_order_is_correct()
  {
    // order = max(diffusion_order_in, diffusion_order_out)
    //       + max(weight_order_in, weight_order_out)
    //       + max(test_order_in, test_order_out)
    //       + max(ansatz_order_in, ansatz_order_out)
    // With unit diffusion/weight (order 0) and scalar_test_(4), scalar_ansatz_(3):
    // order = 0 + 0 + 4 + 3 = 7
    InnerCouplingType integrand(1., 1.);
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    EXPECT_EQ(7, integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_));
  }

  void dirichlet_coupling_binary_evaluates_correctly()
  {
    // Binary form formula with diffusion = I and symmetry_prefactor = s:
    //   result[i][j] = -1 * (I * grad_phi[j] · n) * psi[i]
    //                  - s * phi[j] * (I * grad_psi[i] · n)
    //   with diffusion = I:
    //   result[i][j] = -(grad_phi[j] · n) * psi[i] - s * phi[j] * (grad_psi[i] · n)
    //
    // Test with s=1 (SIPDG variant).
    const double s = 1.0;
    DirichletCouplingType integrand(s, 1. /*diffusion=I*/);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto& binary = static_cast<BinaryBaseType&>(integrand);
    const auto integrand_order = binary.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> result(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      binary.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto n = intersection.unitOuterNormal(x);
      std::vector<ScalarRangeType> phi, psi;
      std::vector<ScalarJacobianType> grad_phi, grad_psi;
      scalar_ansatz_->evaluate(x_in, phi);
      scalar_test_->evaluate(x_in, psi);
      scalar_ansatz_->jacobians(x_in, grad_phi);
      scalar_test_->jacobians(x_in, grad_psi);
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          const double dphi_dot_n = grad_phi[jj][0] * n;
          const double dpsi_dot_n = grad_psi[ii][0] * n;
          const double expected = -dphi_dot_n * psi[ii][0] - s * phi[jj][0] * dpsi_dot_n;
          EXPECT_NEAR(expected, result[ii][jj], 1e-13);
        }
      }
    }
  }

  void dirichlet_coupling_binary_order_is_correct()
  {
    // order = diffusion_order + test_order + ansatz_order = 0 + 4 + 3 = 7
    DirichletCouplingType integrand(1., 1.);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto& binary = static_cast<BinaryBaseType&>(integrand);
    EXPECT_EQ(7, binary.order(*scalar_test_, *scalar_ansatz_));
  }

  void dirichlet_coupling_unary_evaluates_correctly()
  {
    // Unary form formula with diffusion = I, dirichlet_data = g, symmetry_prefactor = s:
    //   result[i] = -s * g(x) * (I * grad_psi[i] · n)
    //             = -s * g(x) * (grad_psi[i] · n)
    //
    // Test with s=1, g=2 (constant Dirichlet data).
    const double s = 1.0;
    const double g_val = 2.0;
    DirichletCouplingType integrand(s, 1. /*diffusion=I*/, g_val /*dirichlet_data*/);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto& unary = static_cast<UnaryBaseType&>(integrand);
    const auto integrand_order = unary.order(*scalar_test_);
    DynamicVector<D> result(2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      unary.evaluate(*scalar_test_, x, result);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto n = intersection.unitOuterNormal(x);
      std::vector<ScalarJacobianType> grad_psi;
      scalar_test_->jacobians(x_in, grad_psi);
      for (size_t ii = 0; ii < 2; ++ii) {
        const double dpsi_dot_n = grad_psi[ii][0] * n;
        const double expected = -s * g_val * dpsi_dot_n;
        EXPECT_NEAR(expected, result[ii], 1e-13);
      }
    }
  }

  void dirichlet_coupling_unary_zero_prefactor_gives_zero()
  {
    // With s=0 (IIPDG): unary result should be 0.
    DirichletCouplingType integrand(0. /*s=0*/, 1. /*diffusion=I*/, 3.0 /*non-trivial data*/);
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto& unary = static_cast<UnaryBaseType&>(integrand);
    const auto center = FieldVector<D, d - 1>(0.5);
    DynamicVector<D> result(2, 0.);
    unary.evaluate(*scalar_test_, center, result);
    for (size_t ii = 0; ii < 2; ++ii)
      EXPECT_NEAR(0., result[ii], 1e-15);
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
}; // struct LaplaceIPDGIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using LaplaceIPDGIntegrandTest = Dune::GDT::Test::LaplaceIPDGIntegrandTest<G>;
TYPED_TEST_SUITE(LaplaceIPDGIntegrandTest, Grids2D);

TYPED_TEST(LaplaceIPDGIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, inner_coupling_post_bind_throws_on_boundary_intersection)
{
  this->inner_coupling_post_bind_throws_on_boundary_intersection();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, inner_coupling_order_is_correct)
{
  this->inner_coupling_order_is_correct();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, inner_coupling_evaluates_correctly_nipdg)
{
  this->inner_coupling_evaluates_correctly_nipdg();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, inner_coupling_evaluates_correctly_iipdg)
{
  this->inner_coupling_evaluates_correctly_iipdg();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, inner_coupling_evaluates_correctly_sipdg)
{
  this->inner_coupling_evaluates_correctly_sipdg();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, dirichlet_coupling_binary_evaluates_correctly)
{
  this->dirichlet_coupling_binary_evaluates_correctly();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, dirichlet_coupling_binary_order_is_correct)
{
  this->dirichlet_coupling_binary_order_is_correct();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, dirichlet_coupling_unary_evaluates_correctly)
{
  this->dirichlet_coupling_unary_evaluates_correctly();
}

TYPED_TEST(LaplaceIPDGIntegrandTest, dirichlet_coupling_unary_zero_prefactor_gives_zero)
{
  this->dirichlet_coupling_unary_zero_prefactor_gives_zero();
}
