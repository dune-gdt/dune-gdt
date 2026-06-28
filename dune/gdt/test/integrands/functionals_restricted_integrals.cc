// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/common/dynvector.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/functions/generic/element-function.hh>

#include <dune/gdt/local/functionals/restricted-integrals.hh>
#include <dune/gdt/local/functionals/integrals.hh>
#include <dune/gdt/local/integrands/generic.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct RestrictedIntersectionFunctionalTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using BaseType::grid_provider_;
  using BaseType::scalar_test_;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;

  using UnrestrictedFunctional = LocalIntersectionIntegralFunctional<I, 1>;
  using RestrictedFunctional = LocalIntersectionRestrictedIntegralFunctional<I, 1>;
  using FilterType = typename RestrictedFunctional::FilterType;
  using BaseType::make_const_basis;

  // A trivial constant unary intersection integrand: evaluates to 1.0 for every basis function.
  auto make_constant_unary_integrand() const
  {
    using Integrand = GenericLocalUnaryIntersectionIntegrand<I>;
    return Integrand(
        // order
        [](const auto& /*basis*/, const auto& /*param*/) { return 0; },
        // evaluate: set all entries to 1
        [](const auto& basis, const auto& /*x*/, DynamicVector<double>& result, const auto& /*param*/) {
          const size_t sz = basis.size();
          if (result.size() < sz)
            result.resize(sz);
          for (size_t ii = 0; ii < sz; ++ii)
            result[ii] = 1.0;
        });
  }

  void is_constructable() final
  {
    auto integrand = make_constant_unary_integrand();

    FilterType accept_all = [](const I&, const auto&) { return true; };
    FilterType reject_all = [](const I&, const auto&) { return false; };

    [[maybe_unused]] RestrictedFunctional func1(accept_all, integrand);
    [[maybe_unused]] RestrictedFunctional func2(accept_all, integrand, /*over_integrate=*/0);
    [[maybe_unused]] RestrictedFunctional func3(accept_all, integrand, /*over_integrate=*/2);
    [[maybe_unused]] RestrictedFunctional func4(reject_all, integrand);

    // Copy constructor
    RestrictedFunctional orig(accept_all, integrand);
    [[maybe_unused]] RestrictedFunctional copied(orig);

    // copy() virtual method
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  void inside_returns_correct_value()
  {
    // The functional delegates inside() to the integrand's inside().
    // GenericLocalUnaryIntersectionIntegrand defaults to inside=true.
    auto integrand = make_constant_unary_integrand();
    FilterType accept_all = [](const I&, const auto&) { return true; };
    RestrictedFunctional functional(accept_all, integrand);
    EXPECT_TRUE(functional.inside());
  }

  void accept_all_filter_equals_unrestricted()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedFunctional unrestricted(integrand);
    RestrictedFunctional restricted(accept_all, integrand);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result_u(1, 0.), result_r(1, 0.);

    for (auto&& intersection : intersections(gv, element)) {
      unrestricted.apply(intersection, *test_basis, result_u);
      restricted.apply(intersection, *test_basis, result_r);
      EXPECT_DOUBLE_EQ(result_u[0], result_r[0])
          << "accept-all restricted functional must equal unrestricted functional";
    }
  }

  void reject_all_filter_gives_zero()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedFunctional restricted(reject_all, integrand);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result(1, 99.0); // pre-filled to verify it is cleared

    for (auto&& intersection : intersections(gv, element)) {
      restricted.apply(intersection, *test_basis, result);
      EXPECT_DOUBLE_EQ(0.0, result[0]) << "reject-all filter must produce zero vector";
    }
  }

  void multi_dof_accept_all_equals_unrestricted()
  {
    // Use the size-2 scalar_test_ from the fixture.
    auto integrand = make_constant_unary_integrand();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    UnrestrictedFunctional unrestricted(integrand);
    RestrictedFunctional restricted(accept_all, integrand);

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    scalar_test_->bind(element);

    DynamicVector<double> result_u(2, 0.), result_r(2, 0.);

    for (auto&& intersection : intersections(gv, element)) {
      unrestricted.apply(intersection, *scalar_test_, result_u);
      restricted.apply(intersection, *scalar_test_, result_r);
      for (size_t ii = 0; ii < 2; ++ii)
        EXPECT_DOUBLE_EQ(result_u[ii], result_r[ii]);
    }
  }

  void multi_dof_reject_all_gives_zero()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedFunctional restricted(reject_all, integrand);

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    scalar_test_->bind(element);

    DynamicVector<double> result(2, 99.0);

    for (auto&& intersection : intersections(gv, element)) {
      restricted.apply(intersection, *scalar_test_, result);
      for (size_t ii = 0; ii < 2; ++ii)
        EXPECT_DOUBLE_EQ(0.0, result[ii]);
    }
  }

  void over_integrate_does_not_change_result_for_constant_integrand()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType accept_all = [](const I&, const auto&) { return true; };

    RestrictedFunctional form_no_over(accept_all, integrand, /*over_integrate=*/0);
    RestrictedFunctional form_over2(accept_all, integrand, /*over_integrate=*/2);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result0(1, 0.), result2(1, 0.);

    for (auto&& intersection : intersections(gv, element)) {
      form_no_over.apply(intersection, *test_basis, result0);
      form_over2.apply(intersection, *test_basis, result2);
      EXPECT_DOUBLE_EQ(result0[0], result2[0])
          << "over_integrate should not change the result for a constant integrand";
    }
  }

  // The integral over an intersection of the constant function 1 should equal the
  // measure (length) of that intersection, regardless of over_integrate.
  void constant_integrand_result_equals_intersection_measure()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType accept_all = [](const I&, const auto&) { return true; };
    RestrictedFunctional restricted(accept_all, integrand);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result(1, 0.);

    for (auto&& intersection : intersections(gv, element)) {
      restricted.apply(intersection, *test_basis, result);
      // The constant-1 integrand integrates to the intersection's volume/length.
      const double expected = intersection.geometry().volume();
      EXPECT_NEAR(expected, result[0], 1e-13) << "constant-1 integrand must integrate to intersection measure";
    }
  }

  // Same as above but with reject-all: result should always be 0, regardless of
  // the intersection measure.
  void reject_all_gives_zero_independent_of_intersection_size()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType reject_all = [](const I&, const auto&) { return false; };
    RestrictedFunctional restricted(reject_all, integrand);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result(1, 99.0);

    for (auto&& intersection : intersections(gv, element)) {
      restricted.apply(intersection, *test_basis, result);
      EXPECT_DOUBLE_EQ(0.0, result[0]);
    }
  }

  // Verify that a partial filter accepts roughly half the quadrature mass.
  void partial_filter_result_is_between_zero_and_unrestricted()
  {
    auto integrand = make_constant_unary_integrand();
    FilterType half_filter = [](const I&, const FieldVector<double, 1>& x) { return x[0] < 0.5; };
    FilterType accept_all = [](const I&, const auto&) { return true; };

    RestrictedFunctional form_half(half_filter, integrand);
    RestrictedFunctional form_all(accept_all, integrand);

    auto test_basis = make_const_basis();

    const auto& gv = grid_provider_->leaf_view();
    const auto& element = *(gv.template begin<0>());
    test_basis->bind(element);

    DynamicVector<double> result_half(1, 0.), result_all(1, 0.);

    for (auto&& intersection : intersections(gv, element)) {
      form_half.apply(intersection, *test_basis, result_half);
      form_all.apply(intersection, *test_basis, result_all);
      EXPECT_GE(result_half[0], 0.0);
      EXPECT_LE(result_half[0], result_all[0] + 1e-15);
    }
  }
}; // struct RestrictedIntersectionFunctionalTest


} // namespace Test
} // namespace GDT
} // namespace Dune


template <class G>
using RestrictedIntersectionFunctionalTest = Dune::GDT::Test::RestrictedIntersectionFunctionalTest<G>;
TYPED_TEST_SUITE(RestrictedIntersectionFunctionalTest, Grids2D);

TYPED_TEST(RestrictedIntersectionFunctionalTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, inside_returns_correct_value)
{
  this->inside_returns_correct_value();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, accept_all_filter_equals_unrestricted)
{
  this->accept_all_filter_equals_unrestricted();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, reject_all_filter_gives_zero)
{
  this->reject_all_filter_gives_zero();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, multi_dof_accept_all_equals_unrestricted)
{
  this->multi_dof_accept_all_equals_unrestricted();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, multi_dof_reject_all_gives_zero)
{
  this->multi_dof_reject_all_gives_zero();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, over_integrate_does_not_change_result_for_constant_integrand)
{
  this->over_integrate_does_not_change_result_for_constant_integrand();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, constant_integrand_result_equals_intersection_measure)
{
  this->constant_integrand_result_equals_intersection_measure();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, reject_all_gives_zero_independent_of_intersection_size)
{
  this->reject_all_gives_zero_independent_of_intersection_size();
}

TYPED_TEST(RestrictedIntersectionFunctionalTest, partial_filter_result_is_between_zero_and_unrestricted)
{
  this->partial_filter_result_is_between_zero_and_unrestricted();
}
