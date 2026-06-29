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

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/functions/generic/grid-function.hh>

#include <dune/gdt/local/integrands/linear-advection-upwind.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct LinearAdvectionUpwindIntegrandTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::DomainType;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;
  using typename BaseType::LocalScalarBasisType;
  using typename BaseType::ScalarJacobianType;
  using typename BaseType::ScalarRangeType;
  using InnerCouplingType = LocalLinearAdvectionUpwindIntegrands::InnerCoupling<I>;
  using DirichletCouplingType = LocalLinearAdvectionUpwindIntegrands::DirichletCoupling<I>;

  // Basis functions non-zero on every face of the reference element.
  // simple_test_:   { 1,   x[0]+x[1]       }  (order 1)
  // simple_ansatz_: { 2,   1+x[0]*x[1]     }  (order 2)
  // Used for intersection tests to avoid zero results at element boundaries.
  std::shared_ptr<LocalScalarBasisType> simple_test_;
  std::shared_ptr<LocalScalarBasisType> simple_ansatz_;

  void SetUp() override
  {
    BaseType::SetUp();
    simple_test_ = std::make_shared<LocalScalarBasisType>(
        2,
        1,
        [](const DomainType& x, std::vector<ScalarRangeType>& ret, const XT::Common::Parameter&) {
          ret.resize(2);
          ret[0] = {1.};
          ret[1] = {x[0] + x[1]};
        },
        XT::Common::ParameterType{},
        [](const DomainType& x, std::vector<ScalarJacobianType>& ret, const XT::Common::Parameter&) {
          ret.resize(2);
          ret[0][0] = {0., 0.};
          ret[1][0] = {1., 1.};
        });
    simple_ansatz_ = std::make_shared<LocalScalarBasisType>(
        2,
        2,
        [](const DomainType& x, std::vector<ScalarRangeType>& ret, const XT::Common::Parameter&) {
          ret.resize(2);
          ret[0] = {2.};
          ret[1] = {1. + x[0] * x[1]};
        },
        XT::Common::ParameterType{},
        [](const DomainType& x, std::vector<ScalarJacobianType>& ret, const XT::Common::Parameter&) {
          ret.resize(2);
          ret[0][0] = {0., 0.};
          ret[1][0] = {x[1], x[0]};
        });
  }

  void is_constructable() override final
  {
    const XT::Functions::GenericGridFunction<E, d> velocity_gf(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    const XT::Functions::GenericFunction<d, d> velocity_func(
        0, [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    const XT::Functions::GenericGridFunction<E, 1> dirichlet_gf(
        0, [](const E&) {}, [](const DomainType&, const XT::Common::Parameter&) -> D { return 42.; });

    // InnerCoupling: grid-function and static-function velocities
    [[maybe_unused]] InnerCouplingType inner1(velocity_gf);
    [[maybe_unused]] InnerCouplingType inner2(velocity_func);
    // InnerCoupling: copy constructor
    [[maybe_unused]] InnerCouplingType inner3(inner1);
    // InnerCoupling: copy_as_quaternary_intersection_integrand
    auto cloned_inner = inner1.copy_as_quaternary_intersection_integrand();
    EXPECT_NE(nullptr, cloned_inner);

    // DirichletCoupling: grid-function velocity, with and without dirichlet data
    [[maybe_unused]] DirichletCouplingType dirichlet1(velocity_gf);
    [[maybe_unused]] DirichletCouplingType dirichlet2(velocity_gf, 0.);
    [[maybe_unused]] DirichletCouplingType dirichlet3(velocity_gf, dirichlet_gf);
    // DirichletCoupling: static-function velocity
    [[maybe_unused]] DirichletCouplingType dirichlet4(velocity_func);
    // DirichletCoupling: copy constructor
    [[maybe_unused]] DirichletCouplingType dirichlet5(dirichlet1);
    // DirichletCoupling: copy_as_unary and copy_as_binary
    auto cloned_unary = dirichlet1.copy_as_unary_intersection_integrand();
    EXPECT_NE(nullptr, cloned_unary);
    auto cloned_binary = dirichlet1.copy_as_binary_intersection_integrand();
    EXPECT_NE(nullptr, cloned_binary);
  }

  void inner_coupling_throws_on_boundary_intersection()
  {
    // InnerCoupling::post_bind() must throw when given a boundary intersection.
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    InnerCouplingType integrand(velocity);
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (intersection.neighbor())
          continue;
        found = true;
        EXPECT_THROW(integrand.bind(intersection), Dune::Exception);
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No boundary intersection found — test is vacuous";
  }

  void inner_coupling_order_is_correct()
  {
    // order = direction.order + test_inside.order + max(ansatz_inside.order, ansatz_outside.order)
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    InnerCouplingType integrand(velocity);
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (!intersection.neighbor())
          continue;
        found = true;
        integrand.bind(intersection);
        // direction order 0, simple_test_ order 1, simple_ansatz_ order 2
        EXPECT_EQ(0 + 1 + 2, integrand.order(*simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_));
        // with fixture bases: direction 0, test 4, ansatz 3
        EXPECT_EQ(0 + 4 + 3, integrand.order(*scalar_test_, *scalar_ansatz_, *scalar_test_, *scalar_ansatz_));
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No interior intersection found in grid";
  }

  void inner_coupling_evaluates_correctly()
  {
    // For the InnerCoupling formula (computed on an interior intersection):
    //   result_in_in  = 0
    //   result_out_in = 0
    //   result_in_out[i][j]  =  (v.n) * ansatz_out[j](x_out) * test_in[i](x_in)
    //   result_out_out[i][j] = -(v.n) * ansatz_out[j](x_out) * test_out[i](x_out)
    //
    // simple_test_   = { 1, x[0]+x[1] }  evaluated at x_in or x_out
    // simple_ansatz_ = { 2, 1+x[0]*x[1] } evaluated at x_out
    // velocity v = (1, 1)
    //
    // We verify the formula for all quadrature points on the first interior intersection.
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 1.}}; });
    InnerCouplingType integrand(velocity);
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (!intersection.neighbor())
          continue;
        found = true;
        integrand.bind(intersection);
        const auto order = integrand.order(*simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_);
        DynamicMatrix<D> res_ii(2, 2, 0.), res_io(2, 2, 0.), res_oi(2, 2, 0.), res_oo(2, 2, 0.);

        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          const auto& x_ref = qp.position();
          const auto x_in = intersection.geometryInInside().global(x_ref);
          const auto x_out = intersection.geometryInOutside().global(x_ref);
          const auto normal = intersection.unitOuterNormal(x_ref);
          const D v_dot_n = normal[0] + normal[1]; // v = (1,1)

          integrand.evaluate(
              *simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_, x_ref, res_ii, res_io, res_oi, res_oo);

          // test values at inside and outside reference points
          const D t_in_0 = 1.;
          const D t_in_1 = x_in[0] + x_in[1];
          const D t_out_0 = 1.;
          const D t_out_1 = x_out[0] + x_out[1];
          // ansatz values at outside reference point
          const D a_out_0 = 2.;
          const D a_out_1 = 1. + x_out[0] * x_out[1];

          assert_inner_coupling_result(
              v_dot_n, t_in_0, t_in_1, t_out_0, t_out_1, a_out_0, a_out_1, res_ii, res_io, res_oi, res_oo);
        }
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No interior intersection found in grid!";
  }

  void inner_coupling_covers_both_sign_orientations()
  {
    // Test with v=(1,0) and v=(-1,0) on the same interior intersection so that
    // both v.n > 0 (outflow) and v.n < 0 (inflow) orientations are exercised.
    // The InnerCoupling formula is the same regardless of sign; only (v.n) changes.
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (!intersection.neighbor())
          continue;
        found = true;

        for (const D vx : {1., -1.}) {
          const XT::Functions::GenericGridFunction<E, d> velocity(
              0,
              [](const E&) {},
              // Lambda captures vx by value; capture list must be explicit.
              [vx](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{vx, 0.}}; });
          InnerCouplingType integrand(velocity);
          integrand.bind(intersection);
          const auto order = integrand.order(*simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_);
          DynamicMatrix<D> res_ii(2, 2, 0.), res_io(2, 2, 0.), res_oi(2, 2, 0.), res_oo(2, 2, 0.);

          for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
            const auto& x_ref = qp.position();
            const auto x_in = intersection.geometryInInside().global(x_ref);
            const auto x_out = intersection.geometryInOutside().global(x_ref);
            const auto normal = intersection.unitOuterNormal(x_ref);
            const D v_dot_n = vx * normal[0]; // v = (vx, 0)

            integrand.evaluate(
                *simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_, x_ref, res_ii, res_io, res_oi, res_oo);

            const D t_in_0 = 1., t_in_1 = x_in[0] + x_in[1];
            const D t_out_0 = 1., t_out_1 = x_out[0] + x_out[1];
            const D a_out_0 = 2., a_out_1 = 1. + x_out[0] * x_out[1];

            assert_inner_coupling_result(
                v_dot_n, t_in_0, t_in_1, t_out_0, t_out_1, a_out_0, a_out_1, res_ii, res_io, res_oi, res_oo);
          }
        }
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No interior intersection found in grid!";
  }

  void inner_coupling_with_zero_velocity_gives_zero()
  {
    // Edge case: v = 0 => v.n = 0 for all intersections => all result matrices are zero.
    const XT::Functions::GenericGridFunction<E, d> zero_vel(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{0., 0.}}; });
    InnerCouplingType integrand(zero_vel);
    const auto& gv = grid_provider_->leaf_view();
    DynamicMatrix<D> res_ii(2, 2, 0.), res_io(2, 2, 0.), res_oi(2, 2, 0.), res_oo(2, 2, 0.);
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (!intersection.neighbor())
          continue;
        integrand.bind(intersection);
        const auto order = integrand.order(*simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_);
        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          integrand.evaluate(*simple_test_,
                             *simple_ansatz_,
                             *simple_test_,
                             *simple_ansatz_,
                             qp.position(),
                             res_ii,
                             res_io,
                             res_oi,
                             res_oo);
          for (size_t ii = 0; ii < 2; ++ii) {
            for (size_t jj = 0; jj < 2; ++jj) {
              EXPECT_DOUBLE_EQ(0., res_ii[ii][jj]);
              EXPECT_DOUBLE_EQ(0., res_io[ii][jj]);
              EXPECT_DOUBLE_EQ(0., res_oi[ii][jj]);
              EXPECT_DOUBLE_EQ(0., res_oo[ii][jj]);
            }
          }
        }
        return; // one intersection is enough
      }
    }
  }

  void inner_coupling_clone_gives_same_results()
  {
    // copy constructor and copy_as_quaternary_intersection_integrand should produce identical results
    const XT::Functions::GenericGridFunction<E, d> velocity(
        1,
        [](const E&) {},
        [](const DomainType& x, const XT::Common::Parameter&) { return FieldVector<D, d>{{x[0], x[1]}}; });
    InnerCouplingType original(velocity);
    InnerCouplingType copy_ctor(original);
    auto cloned = original.copy_as_quaternary_intersection_integrand();

    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (!intersection.neighbor())
          continue;
        original.bind(intersection);
        copy_ctor.bind(intersection);
        cloned->bind(intersection);

        const auto order = original.order(*simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_);
        DynamicMatrix<D> r_ii_o(2, 2, 0.), r_io_o(2, 2, 0.), r_oi_o(2, 2, 0.), r_oo_o(2, 2, 0.);
        DynamicMatrix<D> r_ii_c(2, 2, 0.), r_io_c(2, 2, 0.), r_oi_c(2, 2, 0.), r_oo_c(2, 2, 0.);
        DynamicMatrix<D> r_ii_l(2, 2, 0.), r_io_l(2, 2, 0.), r_oi_l(2, 2, 0.), r_oo_l(2, 2, 0.);

        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          const auto& x_ref = qp.position();
          original.evaluate(
              *simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_, x_ref, r_ii_o, r_io_o, r_oi_o, r_oo_o);
          copy_ctor.evaluate(
              *simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_, x_ref, r_ii_c, r_io_c, r_oi_c, r_oo_c);
          cloned->evaluate(
              *simple_test_, *simple_ansatz_, *simple_test_, *simple_ansatz_, x_ref, r_ii_l, r_io_l, r_oi_l, r_oo_l);

          for (size_t ii = 0; ii < 2; ++ii) {
            for (size_t jj = 0; jj < 2; ++jj) {
              EXPECT_DOUBLE_EQ(r_ii_o[ii][jj], r_ii_c[ii][jj]);
              EXPECT_DOUBLE_EQ(r_ii_o[ii][jj], r_ii_l[ii][jj]);
              EXPECT_DOUBLE_EQ(r_io_o[ii][jj], r_io_c[ii][jj]);
              EXPECT_DOUBLE_EQ(r_io_o[ii][jj], r_io_l[ii][jj]);
              EXPECT_DOUBLE_EQ(r_oi_o[ii][jj], r_oi_c[ii][jj]);
              EXPECT_DOUBLE_EQ(r_oi_o[ii][jj], r_oi_l[ii][jj]);
              EXPECT_DOUBLE_EQ(r_oo_o[ii][jj], r_oo_c[ii][jj]);
              EXPECT_DOUBLE_EQ(r_oo_o[ii][jj], r_oo_l[ii][jj]);
            }
          }
        }
        return;
      }
    }
    ADD_FAILURE() << "No interior intersection found in grid";
  }

  void dirichlet_coupling_inside_is_true()
  {
    // DirichletCoupling::inside() always returns true
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 0.}}; });
    DirichletCouplingType integrand(velocity);
    EXPECT_TRUE(integrand.inside());
  }

  void dirichlet_coupling_evaluates_binary_correctly()
  {
    // DirichletCoupling binary evaluate on a boundary intersection:
    //   result[i][j] = (v.n) * ansatz[j](x_in) * test[i](x_in)
    // Both test and ansatz are evaluated at the inside reference point x_in.
    //
    // simple_test_   = { 1,   x_in[0]+x_in[1]   }
    // simple_ansatz_ = { 2,   1+x_in[0]*x_in[1] }
    // velocity v = (1,1)
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 1.}}; });
    DirichletCouplingType integrand(velocity);
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (intersection.neighbor())
          continue;
        found = true;
        integrand.bind(intersection);
        const auto order = integrand.order(*simple_test_, *simple_ansatz_);
        DynamicMatrix<D> result(2, 2, 0.);

        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          const auto& x_ref = qp.position();
          const auto x_in = intersection.geometryInInside().global(x_ref);
          const auto normal = intersection.unitOuterNormal(x_ref);
          const D v_dot_n = normal[0] + normal[1]; // v = (1,1)

          integrand.evaluate(*simple_test_, *simple_ansatz_, x_ref, result);

          // Both test and ansatz evaluated at x_in
          const D t_0 = 1.;
          const D t_1 = x_in[0] + x_in[1];
          const D a_0 = 2.;
          const D a_1 = 1. + x_in[0] * x_in[1];

          EXPECT_NEAR(v_dot_n * a_0 * t_0, result[0][0], 1e-13);
          EXPECT_NEAR(v_dot_n * a_1 * t_0, result[0][1], 1e-13);
          EXPECT_NEAR(v_dot_n * a_0 * t_1, result[1][0], 1e-13);
          EXPECT_NEAR(v_dot_n * a_1 * t_1, result[1][1], 1e-13);
        }
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No boundary intersection found in grid";
  }

  void dirichlet_coupling_evaluates_unary_correctly()
  {
    // DirichletCoupling unary evaluate on a boundary intersection:
    //   result[j] = (v.n) * dirichlet_data * test[j](x_in)
    const XT::Functions::GenericGridFunction<E, d> velocity(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{1., 1.}}; });
    const D dirichlet_val = 3.;
    const XT::Functions::GenericGridFunction<E, 1> dirichlet_gf(
        0,
        [](const E&) {},
        [dirichlet_val](const DomainType&, const XT::Common::Parameter&) -> D { return dirichlet_val; });
    DirichletCouplingType integrand(velocity, dirichlet_gf);
    const auto& gv = grid_provider_->leaf_view();
    bool found = false;
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (intersection.neighbor())
          continue;
        found = true;
        integrand.bind(intersection);
        const auto order = integrand.order(*simple_test_);
        DynamicVector<D> result(2, 0.);

        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          const auto& x_ref = qp.position();
          const auto x_in = intersection.geometryInInside().global(x_ref);
          const auto normal = intersection.unitOuterNormal(x_ref);
          const D v_dot_n = normal[0] + normal[1];

          integrand.evaluate(*simple_test_, x_ref, result);

          const D t_0 = 1.;
          const D t_1 = x_in[0] + x_in[1];

          EXPECT_NEAR(v_dot_n * dirichlet_val * t_0, result[0], 1e-13);
          EXPECT_NEAR(v_dot_n * dirichlet_val * t_1, result[1], 1e-13);
        }
        break;
      }
      if (found)
        break;
    }
    EXPECT_TRUE(found) << "No boundary intersection found in grid";
  }

  void dirichlet_coupling_with_zero_velocity_gives_zero()
  {
    // Edge case: v = 0 => all results are zero
    const XT::Functions::GenericGridFunction<E, d> zero_vel(
        0,
        [](const E&) {},
        [](const DomainType&, const XT::Common::Parameter&) { return FieldVector<D, d>{{0., 0.}}; });
    DirichletCouplingType integrand(zero_vel, 5.);
    const auto& gv = grid_provider_->leaf_view();
    DynamicMatrix<D> bin_result(2, 2, 0.);
    DynamicVector<D> un_result(2, 0.);
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (intersection.neighbor())
          continue;
        integrand.bind(intersection);
        const auto order = integrand.order(*simple_test_, *simple_ansatz_);
        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          integrand.evaluate(*simple_test_, *simple_ansatz_, qp.position(), bin_result);
          integrand.evaluate(*simple_test_, qp.position(), un_result);
          for (size_t ii = 0; ii < 2; ++ii) {
            EXPECT_DOUBLE_EQ(0., un_result[ii]);
            for (size_t jj = 0; jj < 2; ++jj) {
              EXPECT_DOUBLE_EQ(0., bin_result[ii][jj]);
            }
          }
        }
        return;
      }
    }
  }

  void dirichlet_coupling_clone_gives_same_results()
  {
    // copy constructor and copy_as_* should produce identical binary evaluate results
    const XT::Functions::GenericGridFunction<E, d> velocity(
        1,
        [](const E&) {},
        [](const DomainType& x, const XT::Common::Parameter&) { return FieldVector<D, d>{{x[0], x[1]}}; });
    const XT::Functions::GenericGridFunction<E, 1> dirichlet_gf(
        1, [](const E&) {}, [](const DomainType& x, const XT::Common::Parameter&) -> D { return x[0] + x[1]; });
    DirichletCouplingType original(velocity, dirichlet_gf);
    DirichletCouplingType copy_ctor(original);
    auto cloned_unary = original.copy_as_unary_intersection_integrand();
    auto cloned_binary = original.copy_as_binary_intersection_integrand();

    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : elements(gv)) {
      for (const auto& intersection : intersections(gv, element)) {
        if (intersection.neighbor())
          continue;
        original.bind(intersection);
        copy_ctor.bind(intersection);
        cloned_unary->bind(intersection);
        cloned_binary->bind(intersection);

        // Compare binary results
        const auto order = original.order(*simple_test_, *simple_ansatz_);
        DynamicMatrix<D> r_orig(2, 2, 0.), r_copy(2, 2, 0.), r_clone(2, 2, 0.);
        for (const auto& qp : Dune::QuadratureRules<D, d - 1>::rule(intersection.type(), std::max(order, 2))) {
          const auto& x_ref = qp.position();
          original.evaluate(*simple_test_, *simple_ansatz_, x_ref, r_orig);
          copy_ctor.evaluate(*simple_test_, *simple_ansatz_, x_ref, r_copy);
          cloned_binary->evaluate(*simple_test_, *simple_ansatz_, x_ref, r_clone);
          for (size_t ii = 0; ii < 2; ++ii) {
            for (size_t jj = 0; jj < 2; ++jj) {
              EXPECT_DOUBLE_EQ(r_orig[ii][jj], r_copy[ii][jj]);
              EXPECT_DOUBLE_EQ(r_orig[ii][jj], r_clone[ii][jj]);
            }
          }
        }
        return;
      }
    }
    ADD_FAILURE() << "No boundary intersection found in grid";
  }

  // Shared assertion helper for InnerCoupling: checks all four result matrices at one quadrature point.
  void assert_inner_coupling_result(D v_dot_n,
                                    D t_in_0,
                                    D t_in_1,
                                    D t_out_0,
                                    D t_out_1,
                                    D a_out_0,
                                    D a_out_1,
                                    const DynamicMatrix<D>& res_ii,
                                    const DynamicMatrix<D>& res_io,
                                    const DynamicMatrix<D>& res_oi,
                                    const DynamicMatrix<D>& res_oo)
  {
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj) {
        EXPECT_NEAR(0., res_ii[ii][jj], 1e-13);
        EXPECT_NEAR(0., res_oi[ii][jj], 1e-13);
      }
    EXPECT_NEAR(v_dot_n * a_out_0 * t_in_0, res_io[0][0], 1e-13);
    EXPECT_NEAR(v_dot_n * a_out_1 * t_in_0, res_io[0][1], 1e-13);
    EXPECT_NEAR(v_dot_n * a_out_0 * t_in_1, res_io[1][0], 1e-13);
    EXPECT_NEAR(v_dot_n * a_out_1 * t_in_1, res_io[1][1], 1e-13);
    EXPECT_NEAR(-v_dot_n * a_out_0 * t_out_0, res_oo[0][0], 1e-13);
    EXPECT_NEAR(-v_dot_n * a_out_1 * t_out_0, res_oo[0][1], 1e-13);
    EXPECT_NEAR(-v_dot_n * a_out_0 * t_out_1, res_oo[1][0], 1e-13);
    EXPECT_NEAR(-v_dot_n * a_out_1 * t_out_1, res_oo[1][1], 1e-13);
  }

  using BaseType::grid_provider_;
  using BaseType::is_simplex_grid_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
}; // struct LinearAdvectionUpwindIntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using LinearAdvectionUpwindIntegrandTest = Dune::GDT::Test::LinearAdvectionUpwindIntegrandTest<G>;
TYPED_TEST_SUITE(LinearAdvectionUpwindIntegrandTest, Grids2D);

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_throws_on_boundary_intersection)
{
  this->inner_coupling_throws_on_boundary_intersection();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_order_is_correct)
{
  this->inner_coupling_order_is_correct();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_evaluates_correctly)
{
  this->inner_coupling_evaluates_correctly();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_covers_both_sign_orientations)
{
  this->inner_coupling_covers_both_sign_orientations();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_with_zero_velocity_gives_zero)
{
  this->inner_coupling_with_zero_velocity_gives_zero();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, inner_coupling_clone_gives_same_results)
{
  this->inner_coupling_clone_gives_same_results();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, dirichlet_coupling_inside_is_true)
{
  this->dirichlet_coupling_inside_is_true();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, dirichlet_coupling_evaluates_binary_correctly)
{
  this->dirichlet_coupling_evaluates_binary_correctly();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, dirichlet_coupling_evaluates_unary_correctly)
{
  this->dirichlet_coupling_evaluates_unary_correctly();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, dirichlet_coupling_with_zero_velocity_gives_zero)
{
  this->dirichlet_coupling_with_zero_velocity_gives_zero();
}

TYPED_TEST(LinearAdvectionUpwindIntegrandTest, dirichlet_coupling_clone_gives_same_results)
{
  this->dirichlet_coupling_clone_gives_same_results();
}
