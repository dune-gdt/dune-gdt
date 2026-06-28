// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/common/dynmatrix.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/functions/generic/element-function.hh>

#include <dune/gdt/local/bilinear-forms/restricted-integrals.hh>
#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/integrands/generic.hh>
#include <dune/gdt/local/integrands/jump.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


// Trivial binary intersection integrand: sets result[i][j] = 1 for all i, j.
// Works for any intersection type and any quadrature point.
template <class I>
auto make_constant_binary_intersection_integrand()
{
  using Integrand = GenericLocalBinaryIntersectionIntegrand<I>;
  return Integrand([](const auto& /*t*/, const auto& /*a*/, const auto& /*p*/) { return 0; },
                   [](const auto& t, const auto& a, const auto& /*x*/, DynamicMatrix<double>& r, const auto& /*p*/) {
                     const size_t rows = t.size();
                     const size_t cols = a.size();
                     if (r.rows() < rows || r.cols() < cols)
                       r.resize(rows, cols);
                     for (size_t ii = 0; ii < rows; ++ii)
                       for (size_t jj = 0; jj < cols; ++jj)
                         r[ii][jj] = 1.0;
                   });
}


// ===========================================================================================
// Tests for LocalIntersectionRestrictedIntegralBilinearForm (single-sided, binary integrand)
// ===========================================================================================

template <class G>
struct SingleSidedRestrictedBilinearFormTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;

  using UnrestrictedBF = LocalIntersectionIntegralBilinearForm<I, 1>;
  using RestrictedBF = LocalIntersectionRestrictedIntegralBilinearForm<I, 1>;
  using FilterType = typename RestrictedBF::FilterType;
  using BaseType::for_each_intersection_of_first_element;
  using BaseType::make_const_basis;

  void is_constructable() final
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType accept_all = [](const I&, const auto&) { return true; };
    FilterType reject_all = [](const I&, const auto&) { return false; };

    [[maybe_unused]] RestrictedBF form1(accept_all, integrand);
    [[maybe_unused]] RestrictedBF form2(accept_all, integrand, /*over_integrate=*/0);
    [[maybe_unused]] RestrictedBF form3(accept_all, integrand, /*over_integrate=*/2);
    [[maybe_unused]] RestrictedBF form4(reject_all, integrand);

    RestrictedBF orig(accept_all, integrand);
    [[maybe_unused]] RestrictedBF copied(orig);
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  void accept_all_filter_equals_unrestricted()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedBF unrestricted(integrand);
    RestrictedBF restricted(accept_all, integrand);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> result_u(1, 1, 0.), result_r(1, 1, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      unrestricted.apply2(is, *test_basis, *ansatz_basis, result_u);
      restricted.apply2(is, *test_basis, *ansatz_basis, result_r);
      EXPECT_DOUBLE_EQ(result_u[0][0], result_r[0][0]) << "accept-all restricted form must equal unrestricted form";
    });
  }

  void reject_all_filter_gives_zero()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedBF restricted(reject_all, integrand);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> result(1, 1, 99.0);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      restricted.apply2(is, *test_basis, *ansatz_basis, result);
      EXPECT_DOUBLE_EQ(0.0, result[0][0]) << "reject-all filter must produce zero matrix";
    });
  }

  void multi_dof_accept_all_equals_unrestricted()
  {
    // Use the size-2 bases from the fixture.
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedBF unrestricted(integrand);
    RestrictedBF restricted(accept_all, integrand);
    DynamicMatrix<double> result_u(2, 2, 0.), result_r(2, 2, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      scalar_test_->bind(el);
      scalar_ansatz_->bind(el);
      unrestricted.apply2(is, *scalar_test_, *scalar_ansatz_, result_u);
      restricted.apply2(is, *scalar_test_, *scalar_ansatz_, result_r);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_DOUBLE_EQ(result_u[ii][jj], result_r[ii][jj]);
    });
  }

  void multi_dof_reject_all_gives_zero()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedBF restricted(reject_all, integrand);
    DynamicMatrix<double> result(2, 2, 99.0);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      scalar_test_->bind(el);
      scalar_ansatz_->bind(el);
      restricted.apply2(is, *scalar_test_, *scalar_ansatz_, result);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_DOUBLE_EQ(0.0, result[ii][jj]);
    });
  }

  // A constant integrand is exact at any non-negative quadrature order, so
  // over_integrate should not change the result.
  void over_integrate_does_not_change_result_for_exact_integrand()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    RestrictedBF form0(accept_all, integrand, /*over_integrate=*/0);
    RestrictedBF form2(accept_all, integrand, /*over_integrate=*/2);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> result0(1, 1, 0.), result2(1, 1, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      form0.apply2(is, *test_basis, *ansatz_basis, result0);
      form2.apply2(is, *test_basis, *ansatz_basis, result2);
      EXPECT_DOUBLE_EQ(result0[0][0], result2[0][0])
          << "over_integrate must not change the result for a constant integrand";
    });
  }

  // Result must equal intersection measure for constant-1 integrand with accept-all filter.
  void constant_integrand_equals_intersection_measure()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType accept_all = [](const I&, const auto&) { return true; };
    RestrictedBF restricted(accept_all, integrand);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> result(1, 1, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      restricted.apply2(is, *test_basis, *ansatz_basis, result);
      EXPECT_NEAR(is.geometry().volume(), result[0][0], 1e-13)
          << "constant-1 integrand must integrate to intersection measure";
    });
  }

  // A partial filter must produce a result in [0, unrestricted_result].
  void partial_filter_result_is_between_zero_and_unrestricted()
  {
    auto integrand = make_constant_binary_intersection_integrand<I>();
    FilterType half = [](const I&, const FieldVector<double, 1>& x) { return x[0] < 0.5; };
    FilterType all = [](const I&, const auto&) { return true; };

    RestrictedBF form_half(half, integrand);
    RestrictedBF form_all(all, integrand);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> res_half(1, 1, 0.), res_all(1, 1, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      form_half.apply2(is, *test_basis, *ansatz_basis, res_half);
      form_all.apply2(is, *test_basis, *ansatz_basis, res_all);
      EXPECT_GE(res_half[0][0], 0.0);
      EXPECT_LE(res_half[0][0], res_all[0][0] + 1e-15);
    });
  }

  // Test with LocalJumpIntegrands::Boundary which maps intersection -> element coordinates.
  void jump_boundary_integrand_accept_all_equals_unrestricted()
  {
    LocalJumpIntegrands::Boundary<I, 1> integrand;
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedBF unrestricted(integrand);
    RestrictedBF restricted(accept_all, integrand);
    auto test_basis = make_const_basis();
    auto ansatz_basis = make_const_basis();
    DynamicMatrix<double> result_u(1, 1, 0.), result_r(1, 1, 0.);

    for_each_intersection_of_first_element([&](const GV&, const E& el, const I& is) {
      test_basis->bind(el);
      ansatz_basis->bind(el);
      unrestricted.apply2(is, *test_basis, *ansatz_basis, result_u);
      restricted.apply2(is, *test_basis, *ansatz_basis, result_r);
      EXPECT_DOUBLE_EQ(result_u[0][0], result_r[0][0]);
    });
  }
}; // struct SingleSidedRestrictedBilinearFormTest


// ===========================================================================================
// Tests for LocalCouplingIntersectionRestrictedIntegralBilinearForm (coupling, quaternary)
// ===========================================================================================

template <class G>
struct CouplingRestrictedBilinearFormTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;

  using LocalQuatIntegrand = LocalJumpIntegrands::Inner<I, 1>;
  using UnrestrictedBF = LocalCouplingIntersectionIntegralBilinearForm<I, 1>;
  using RestrictedBF = LocalCouplingIntersectionRestrictedIntegralBilinearForm<I, 1>;
  using FilterType = typename RestrictedBF::FilterType;
  using BaseType::make_const_basis;
  using BaseType::with_first_coupling_intersection;
  using BaseType::with_first_interior_intersection;

  void is_constructable() final
  {
    LocalQuatIntegrand integrand;
    FilterType accept_all = [](const I&, const auto&) { return true; };
    FilterType reject_all = [](const I&, const auto&) { return false; };

    [[maybe_unused]] RestrictedBF form1(accept_all, integrand);
    [[maybe_unused]] RestrictedBF form2(accept_all, integrand, /*over_integrate=*/0);
    [[maybe_unused]] RestrictedBF form3(accept_all, integrand, /*over_integrate=*/2);
    [[maybe_unused]] RestrictedBF form4(reject_all, integrand);

    RestrictedBF orig(accept_all, integrand);
    [[maybe_unused]] RestrictedBF copied(orig);
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  void accept_all_filter_equals_unrestricted()
  {
    LocalQuatIntegrand integrand;
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedBF unrestricted(integrand);
    RestrictedBF restricted(accept_all, integrand);

    bool found = with_first_coupling_intersection([&](const I& is, auto& basis_in, auto& basis_out) {
      DynamicMatrix<double> u_ii(1, 1), u_io(1, 1), u_oi(1, 1), u_oo(1, 1);
      DynamicMatrix<double> r_ii(1, 1), r_io(1, 1), r_oi(1, 1), r_oo(1, 1);

      unrestricted.apply2(is, basis_in, basis_in, basis_out, basis_out, u_ii, u_io, u_oi, u_oo);
      restricted.apply2(is, basis_in, basis_in, basis_out, basis_out, r_ii, r_io, r_oi, r_oo);

      EXPECT_DOUBLE_EQ(u_ii[0][0], r_ii[0][0]) << "result_in_in must match";
      EXPECT_DOUBLE_EQ(u_io[0][0], r_io[0][0]) << "result_in_out must match";
      EXPECT_DOUBLE_EQ(u_oi[0][0], r_oi[0][0]) << "result_out_in must match";
      EXPECT_DOUBLE_EQ(u_oo[0][0], r_oo[0][0]) << "result_out_out must match";
    });

    ASSERT_TRUE(found) << "Test grid must have at least one interior intersection";
  }

  void reject_all_filter_gives_zero()
  {
    LocalQuatIntegrand integrand;
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedBF restricted(reject_all, integrand);

    bool found = with_first_coupling_intersection([&](const I& is, auto& basis_in, auto& basis_out) {
      DynamicMatrix<double> r_ii(1, 1, 99.), r_io(1, 1, 99.), r_oi(1, 1, 99.), r_oo(1, 1, 99.);
      restricted.apply2(is, basis_in, basis_in, basis_out, basis_out, r_ii, r_io, r_oi, r_oo);

      EXPECT_DOUBLE_EQ(0.0, r_ii[0][0]) << "reject-all: result_in_in must be zero";
      EXPECT_DOUBLE_EQ(0.0, r_io[0][0]) << "reject-all: result_in_out must be zero";
      EXPECT_DOUBLE_EQ(0.0, r_oi[0][0]) << "reject-all: result_out_in must be zero";
      EXPECT_DOUBLE_EQ(0.0, r_oo[0][0]) << "reject-all: result_out_out must be zero";
    });

    ASSERT_TRUE(found);
  }

  // Jump integrand is a penalty: diagonal results in_in, out_out must be positive;
  // a partial filter must produce smaller absolute values than the full filter.
  void partial_filter_absolute_values_bounded_by_unrestricted()
  {
    LocalQuatIntegrand integrand;
    FilterType half = [](const I&, const FieldVector<double, 1>& x) { return x[0] < 0.5; };
    FilterType all = [](const I&, const auto&) { return true; };

    RestrictedBF form_half(half, integrand);
    RestrictedBF form_all(all, integrand);

    bool found = with_first_coupling_intersection([&](const I& is, auto& basis_in, auto& basis_out) {
      DynamicMatrix<double> h_ii(1, 1), h_io(1, 1), h_oi(1, 1), h_oo(1, 1);
      DynamicMatrix<double> a_ii(1, 1), a_io(1, 1), a_oi(1, 1), a_oo(1, 1);

      form_half.apply2(is, basis_in, basis_in, basis_out, basis_out, h_ii, h_io, h_oi, h_oo);
      form_all.apply2(is, basis_in, basis_in, basis_out, basis_out, a_ii, a_io, a_oi, a_oo);

      EXPECT_LE(std::abs(h_ii[0][0]), std::abs(a_ii[0][0]) + 1e-14);
      EXPECT_LE(std::abs(h_io[0][0]), std::abs(a_io[0][0]) + 1e-14);
      EXPECT_LE(std::abs(h_oi[0][0]), std::abs(a_oi[0][0]) + 1e-14);
      EXPECT_LE(std::abs(h_oo[0][0]), std::abs(a_oo[0][0]) + 1e-14);
    });

    ASSERT_TRUE(found);
  }

  // Using Inner (quaternary) integrand on a BOUNDARY intersection must throw.
  void inner_integrand_throws_on_boundary_intersection()
  {
    LocalQuatIntegrand integrand;
    FilterType accept_all = [](const I&, const auto&) { return true; };
    RestrictedBF restricted(accept_all, integrand);

    auto basis = make_const_basis();
    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    basis->bind(element);

    DynamicMatrix<double> r(1, 1, 0.);

    bool found_boundary = false;
    for (auto&& intersection : intersections(gv, element)) {
      if (!intersection.neighbor()) {
        found_boundary = true;
        // Inner's post_bind must throw on a boundary intersection.
        EXPECT_THROW(restricted.apply2(intersection, *basis, *basis, *basis, *basis, r, r, r, r), Dune::Exception);
        break;
      }
    }
    if (!found_boundary)
      GTEST_SKIP() << "No boundary intersection found on first element; skipping.";
  }
}; // struct CouplingRestrictedBilinearFormTest


} // namespace Test
} // namespace GDT
} // namespace Dune


// ===========================================================================================
// Typed-test registrations
// ===========================================================================================

template <class G>
using SingleSidedRestrictedBilinearFormTest = Dune::GDT::Test::SingleSidedRestrictedBilinearFormTest<G>;
TYPED_TEST_SUITE(SingleSidedRestrictedBilinearFormTest, Grids2D);

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, accept_all_filter_equals_unrestricted)
{
  this->accept_all_filter_equals_unrestricted();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, reject_all_filter_gives_zero)
{
  this->reject_all_filter_gives_zero();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, multi_dof_accept_all_equals_unrestricted)
{
  this->multi_dof_accept_all_equals_unrestricted();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, multi_dof_reject_all_gives_zero)
{
  this->multi_dof_reject_all_gives_zero();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, over_integrate_does_not_change_result_for_exact_integrand)
{
  this->over_integrate_does_not_change_result_for_exact_integrand();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, constant_integrand_equals_intersection_measure)
{
  this->constant_integrand_equals_intersection_measure();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, partial_filter_result_is_between_zero_and_unrestricted)
{
  this->partial_filter_result_is_between_zero_and_unrestricted();
}

TYPED_TEST(SingleSidedRestrictedBilinearFormTest, jump_boundary_integrand_accept_all_equals_unrestricted)
{
  this->jump_boundary_integrand_accept_all_equals_unrestricted();
}


template <class G>
using CouplingRestrictedBilinearFormTest = Dune::GDT::Test::CouplingRestrictedBilinearFormTest<G>;
TYPED_TEST_SUITE(CouplingRestrictedBilinearFormTest, Grids2D);

TYPED_TEST(CouplingRestrictedBilinearFormTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(CouplingRestrictedBilinearFormTest, accept_all_filter_equals_unrestricted)
{
  this->accept_all_filter_equals_unrestricted();
}

TYPED_TEST(CouplingRestrictedBilinearFormTest, reject_all_filter_gives_zero)
{
  this->reject_all_filter_gives_zero();
}

TYPED_TEST(CouplingRestrictedBilinearFormTest, partial_filter_absolute_values_bounded_by_unrestricted)
{
  this->partial_filter_absolute_values_bounded_by_unrestricted();
}

TYPED_TEST(CouplingRestrictedBilinearFormTest, inner_integrand_throws_on_boundary_intersection)
{
  this->inner_integrand_throws_on_boundary_intersection();
}
