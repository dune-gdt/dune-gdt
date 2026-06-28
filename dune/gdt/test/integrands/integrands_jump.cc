// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/intersection.hh>

#include <dune/gdt/local/integrands/jump.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct JumpIntegrandTest : public IntegrandTest<G>
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
  using typename BaseType::VectorRangeType;

  using ScalarInnerIntegrandType = LocalJumpIntegrands::Inner<I, 1>;
  using VectorInnerIntegrandType = LocalJumpIntegrands::Inner<I, d>;
  using ScalarBoundaryIntegrandType = LocalJumpIntegrands::Boundary<I, 1>;

  // Helper: return first inner intersection found, or throw if none exists.
  I find_inner_intersection() const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : Dune::elements(gv))
      for (const auto& intersection : Dune::intersections(gv, element))
        if (intersection.neighbor())
          return intersection;
    DUNE_THROW(Dune::InvalidStateException, "No inner intersection found in grid!");
  }

  // Helper: return first boundary intersection found, or throw if none exists.
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
    // Inner with default diameter function
    [[maybe_unused]] ScalarInnerIntegrandType inner1;
    // Inner with custom diameter function
    [[maybe_unused]] ScalarInnerIntegrandType inner2([](const I& i) { return XT::Grid::diameter(i); });
    // Inner copy constructor
    ScalarInnerIntegrandType inner3;
    [[maybe_unused]] ScalarInnerIntegrandType inner4(inner3);
    // copy_as_quaternary_intersection_integrand
    auto clone = inner3.copy_as_quaternary_intersection_integrand();
    EXPECT_NE(clone, nullptr);

    // Vector variant
    [[maybe_unused]] VectorInnerIntegrandType vec_inner1;
    [[maybe_unused]] VectorInnerIntegrandType vec_inner2([](const I& i) { return XT::Grid::diameter(i); });

    // Boundary with default diameter function
    [[maybe_unused]] ScalarBoundaryIntegrandType boundary1;
    // Boundary with custom diameter function
    [[maybe_unused]] ScalarBoundaryIntegrandType boundary2([](const I& i) { return XT::Grid::diameter(i); });
    // Boundary copy constructor
    ScalarBoundaryIntegrandType boundary3;
    [[maybe_unused]] ScalarBoundaryIntegrandType boundary4(boundary3);
    // copy_as_binary_intersection_integrand
    auto bclone = boundary3.copy_as_binary_intersection_integrand();
    EXPECT_NE(bclone, nullptr);
    // inside() returns true for Boundary
    EXPECT_TRUE(boundary3.inside());
  }

  void inner_post_bind_throws_on_boundary_intersection()
  {
    ScalarInnerIntegrandType integrand;
    const auto& boundary_intersection = find_boundary_intersection();
    EXPECT_THROW(integrand.bind(boundary_intersection), Dune::Exception);
  }

  void inner_order_is_correct()
  {
    // order = max(test_order_in, test_order_out) + max(ansatz_order_in, ansatz_order_out)
    // scalar_test_ has order 4, scalar_ansatz_ has order 3: expected order = 4 + 3 = 7
    ScalarInnerIntegrandType integrand;
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    EXPECT_EQ(7, integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_));
  }

  void inner_evaluates_correctly_scalar()
  {
    // Formula (r=1): result_in_in[ii][jj] = h * phi_j_in * psi_i_in
    //                result_in_out[ii][jj] = -h * phi_j_out * psi_i_in
    //                result_out_in[ii][jj] = -h * phi_j_in * psi_i_out
    //                result_out_out[ii][jj] = h * phi_j_out * psi_i_out
    // We verify by independently evaluating the scalar_ansatz_ / scalar_test_ bases and
    // re-applying the formula, then comparing with evaluate().
    ScalarInnerIntegrandType integrand;
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const auto h = XT::Grid::diameter(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(
          *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, x, rin_in, rin_out, rout_in, rout_out);
      // Map intersection ref point to element ref points and evaluate bases independently.
      const auto x_in = intersection.geometryInInside().global(x);
      const auto x_out = intersection.geometryInOutside().global(x);
      std::vector<ScalarRangeType> phi_in, phi_out, psi_in, psi_out;
      scalar_ansatz_->evaluate(x_in, phi_in);
      scalar_ansatz_->evaluate(x_out, phi_out);
      scalar_test_->evaluate(x_in, psi_in);
      scalar_test_->evaluate(x_out, psi_out);
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          EXPECT_NEAR(h * phi_in[jj][0] * psi_in[ii][0], rin_in[ii][jj], 1e-13);
          EXPECT_NEAR(-h * phi_out[jj][0] * psi_in[ii][0], rin_out[ii][jj], 1e-13);
          EXPECT_NEAR(-h * phi_in[jj][0] * psi_out[ii][0], rout_in[ii][jj], 1e-13);
          EXPECT_NEAR(h * phi_out[jj][0] * psi_out[ii][0], rout_out[ii][jj], 1e-13);
        }
      }
      // Sign pattern: in-in and out-out are non-negative when phi and psi have same sign;
      // in-out and out-in have opposite sign. Verify block-diagonal sign symmetry.
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj) {
          EXPECT_NEAR(rin_in[ii][jj], rout_out[jj][ii], 1e-13); // symmetric w.r.t. in/out swap
          EXPECT_NEAR(rin_out[ii][jj], rout_in[jj][ii], 1e-13);
        }
    }
  }

  void inner_evaluates_correctly_vector()
  {
    // Formula (r=d): result_in_in[ii][jj] = h * (phi_j_in · n) * (psi_i_in · n)
    //                result_in_out[ii][jj] = -h * (phi_j_out · n) * (psi_i_in · n)
    //                etc.
    VectorInnerIntegrandType integrand;
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const auto h = XT::Grid::diameter(intersection);
    const auto integrand_order = integrand.order(*vector_test_, *vector_ansatz_, *vector_test_, *vector_ansatz_);
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(
          *vector_test_, *vector_ansatz_, *vector_test_, *vector_ansatz_, x, rin_in, rin_out, rout_in, rout_out);
      const auto x_in = intersection.geometryInInside().global(x);
      const auto x_out = intersection.geometryInOutside().global(x);
      const auto n = intersection.unitOuterNormal(x);
      std::vector<VectorRangeType> phi_in, phi_out, psi_in, psi_out;
      vector_ansatz_->evaluate(x_in, phi_in);
      vector_ansatz_->evaluate(x_out, phi_out);
      vector_test_->evaluate(x_in, psi_in);
      vector_test_->evaluate(x_out, psi_out);
      for (size_t ii = 0; ii < 2; ++ii) {
        for (size_t jj = 0; jj < 2; ++jj) {
          const double phi_in_dot_n = phi_in[jj] * n;
          const double phi_out_dot_n = phi_out[jj] * n;
          const double psi_in_dot_n = psi_in[ii] * n;
          const double psi_out_dot_n = psi_out[ii] * n;
          EXPECT_NEAR(h * phi_in_dot_n * psi_in_dot_n, rin_in[ii][jj], 1e-13);
          EXPECT_NEAR(-h * phi_out_dot_n * psi_in_dot_n, rin_out[ii][jj], 1e-13);
          EXPECT_NEAR(-h * phi_in_dot_n * psi_out_dot_n, rout_in[ii][jj], 1e-13);
          EXPECT_NEAR(h * phi_out_dot_n * psi_out_dot_n, rout_out[ii][jj], 1e-13);
        }
      }
    }
  }

  void boundary_inside_returns_true()
  {
    ScalarBoundaryIntegrandType integrand;
    EXPECT_TRUE(integrand.inside());
  }

  void boundary_order_is_correct()
  {
    // order = test_order + ansatz_order = 4 + 3 = 7
    ScalarBoundaryIntegrandType integrand;
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    EXPECT_EQ(7, integrand.order(*scalar_test_, *scalar_ansatz_));
  }

  void boundary_evaluates_correctly()
  {
    // Formula (r=1): result[ii][jj] = h * phi_j * psi_i
    ScalarBoundaryIntegrandType integrand;
    const auto& intersection = find_boundary_intersection();
    integrand.bind(intersection);
    const auto h = XT::Grid::diameter(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<D> result(2, 2, 0.);
    for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order)) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result);
      const auto x_in = intersection.geometryInInside().global(x);
      std::vector<ScalarRangeType> phi, psi;
      scalar_ansatz_->evaluate(x_in, phi);
      scalar_test_->evaluate(x_in, psi);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_NEAR(h * phi[jj][0] * psi[ii][0], result[ii][jj], 1e-13);
    }
  }

  void custom_diameter_function_is_used()
  {
    // Verify that a custom diameter function is actually called.
    double custom_h = 42.0;
    ScalarInnerIntegrandType integrand([custom_h](const I&) { return custom_h; });
    const auto& intersection = find_inner_intersection();
    integrand.bind(intersection);
    const auto integrand_order = integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_);
    // Use a single quadrature point at the center of the intersection.
    DynamicMatrix<D> rin_in(2, 2, 0.), rin_out(2, 2, 0.), rout_in(2, 2, 0.), rout_out(2, 2, 0.);
    const auto center = FieldVector<D, d - 1>(0.5);
    integrand.evaluate(
        *scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_, center, rin_in, rin_out, rout_in, rout_out);
    const auto x_in = intersection.geometryInInside().global(center);
    const auto x_out = intersection.geometryInOutside().global(center);
    std::vector<ScalarRangeType> phi_in, phi_out, psi_in, psi_out;
    scalar_ansatz_->evaluate(x_in, phi_in);
    scalar_ansatz_->evaluate(x_out, phi_out);
    scalar_test_->evaluate(x_in, psi_in);
    scalar_test_->evaluate(x_out, psi_out);
    EXPECT_NEAR(custom_h * phi_in[0][0] * psi_in[0][0], rin_in[0][0], 1e-13);
    EXPECT_NEAR(-custom_h * phi_out[0][0] * psi_in[0][0], rin_out[0][0], 1e-13);
  }

  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using BaseType::vector_ansatz_;
  using BaseType::vector_test_;
}; // struct JumpIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using JumpIntegrandTest = Dune::GDT::Test::JumpIntegrandTest<G>;
TYPED_TEST_SUITE(JumpIntegrandTest, Grids2D);

TYPED_TEST(JumpIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(JumpIntegrandTest, inner_post_bind_throws_on_boundary_intersection)
{
  this->inner_post_bind_throws_on_boundary_intersection();
}

TYPED_TEST(JumpIntegrandTest, inner_order_is_correct)
{
  this->inner_order_is_correct();
}

TYPED_TEST(JumpIntegrandTest, inner_evaluates_correctly_scalar)
{
  this->inner_evaluates_correctly_scalar();
}

TYPED_TEST(JumpIntegrandTest, inner_evaluates_correctly_vector)
{
  this->inner_evaluates_correctly_vector();
}

TYPED_TEST(JumpIntegrandTest, boundary_inside_returns_true)
{
  this->boundary_inside_returns_true();
}

TYPED_TEST(JumpIntegrandTest, boundary_order_is_correct)
{
  this->boundary_order_is_correct();
}

TYPED_TEST(JumpIntegrandTest, boundary_evaluates_correctly)
{
  this->boundary_evaluates_correctly();
}

TYPED_TEST(JumpIntegrandTest, custom_diameter_function_is_used)
{
  this->custom_diameter_function_is_used();
}
