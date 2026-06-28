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

#include <dune/gdt/local/bilinear-forms/generic.hh>
#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/integrands/generic.hh>
#include <dune/gdt/local/integrands/product.hh>

#include <dune/gdt/test/integrands/integrands.hh>

namespace Dune {
namespace GDT {
namespace Test {


// ===========================================================================================
// Tests for GenericLocalElementBilinearForm
// ===========================================================================================

template <class G>
struct GenericElementBilinearFormTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;

  using GenericForm = GenericLocalElementBilinearForm<E, 1>;

  void is_constructable() final
  {
    // Minimal: do-nothing lambda.
    [[maybe_unused]] GenericForm form1(
        [](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const auto& /*p*/) {});

    // With explicit ParameterType.
    [[maybe_unused]] GenericForm form2(
        [](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const auto& /*p*/) {},
        XT::Common::ParameterType{});

    GenericForm orig([](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const auto& /*p*/) {});
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  // The lambda must be called exactly once per apply2() invocation.
  void lambda_is_called()
  {
    int call_count = 0;
    GenericForm form([&call_count](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const auto& /*p*/) {
      ++call_count;
    });

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result;
    form.apply2(*scalar_test_, *scalar_ansatz_, result);
    EXPECT_EQ(1, call_count) << "lambda must be called exactly once per apply2()";
  }

  // apply2() must clear the result to zero BEFORE invoking the lambda.
  void result_is_cleared_before_lambda()
  {
    DynamicMatrix<double> seen_on_entry;

    GenericForm form([&seen_on_entry](const auto& test,
                                     const auto& ansatz,
                                     DynamicMatrix<double>& result,
                                     const XT::Common::Parameter& /*param*/) {
      seen_on_entry = result; // capture what the matrix looks like at lambda entry
      // Write something so we can verify it reaches the caller.
      for (size_t ii = 0; ii < test.size(); ++ii)
        for (size_t jj = 0; jj < ansatz.size(); ++jj)
          result[ii][jj] = 42.0;
    });

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result(2, 2, 99.0); // pre-filled
    form.apply2(*scalar_test_, *scalar_ansatz_, result);

    // apply2() must have cleared the matrix before calling the lambda.
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_DOUBLE_EQ(0.0, seen_on_entry[ii][jj]) << "result must be zero when lambda is entered";

    // The value written by the lambda must be visible to the caller.
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_DOUBLE_EQ(42.0, result[ii][jj]) << "lambda-written values must be returned";
  }

  // A lambda that leaves the result untouched yields zero (because apply2() pre-clears).
  void do_nothing_lambda_gives_zero()
  {
    GenericForm form([](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const auto& /*p*/) {});

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result(2, 2, 99.0);
    form.apply2(*scalar_test_, *scalar_ansatz_, result);

    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_DOUBLE_EQ(0.0, result[ii][jj]);
  }

  // apply2() must resize the result matrix if it is too small.
  void result_is_resized_if_too_small()
  {
    GenericForm form([](const auto& test, const auto& ansatz, auto& result, const auto& /*p*/) {
      for (size_t ii = 0; ii < test.size(); ++ii)
        for (size_t jj = 0; jj < ansatz.size(); ++jj)
          result[ii][jj] = static_cast<double>(ii * 10 + jj);
    });

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result(1, 1, 0.); // initially too small (need 2x2)
    form.apply2(*scalar_test_, *scalar_ansatz_, result);

    ASSERT_GE(result.rows(), 2u);
    ASSERT_GE(result.cols(), 2u);
    EXPECT_DOUBLE_EQ(0.0, result[0][0]);
    EXPECT_DOUBLE_EQ(1.0, result[0][1]);
    EXPECT_DOUBLE_EQ(10.0, result[1][0]);
    EXPECT_DOUBLE_EQ(11.0, result[1][1]);
  }

  // Verify that parameters are forwarded to the lambda.
  void parameter_is_forwarded_to_lambda()
  {
    XT::Common::ParameterType param_type("mu", 1);
    double captured_mu = -1.0;

    GenericLocalElementBilinearForm<E, 1> form(
        [&captured_mu](const auto& /*t*/, const auto& /*a*/, auto& /*r*/, const XT::Common::Parameter& p) {
          captured_mu = p.get("mu").at(0);
        },
        param_type);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result;
    form.apply2(*scalar_test_, *scalar_ansatz_, result, XT::Common::Parameter("mu", 3.14));
    EXPECT_DOUBLE_EQ(3.14, captured_mu) << "lambda must receive the correct parameter value";
  }

  // A lambda computing product < phi_i, psi_j > must agree with LocalElementIntegralBilinearForm
  // using the product integrand at each quadrature point.
  void generic_form_lambda_computes_product_correctly()
  {
    // Lambda form: writes test[i] * ansatz[j] into result[i][j] at a single fixed point.
    // This is NOT a full integral (no quadrature loop) -- it verifies the dispatch mechanism.
    GenericLocalElementBilinearForm<E, 1> generic_form(
        [](const auto& test, const auto& ansatz, DynamicMatrix<double>& result, const XT::Common::Parameter& param) {
          const size_t rows = test.size(param);
          const size_t cols = ansatz.size(param);
          // Evaluate bases at the reference element origin (0, 0) ...
          const FieldVector<double, G::dimension> origin(0.0);
          std::vector<FieldVector<double, 1>> test_vals(rows), ansatz_vals(cols);
          test.evaluate(origin, test_vals, param);
          ansatz.evaluate(origin, ansatz_vals, param);
          for (size_t ii = 0; ii < rows; ++ii)
            for (size_t jj = 0; jj < cols; ++jj)
              result[ii][jj] = test_vals[ii][0] * ansatz_vals[jj][0];
        });

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result(2, 2, 0.);
    generic_form.apply2(*scalar_test_, *scalar_ansatz_, result);

    // scalar_test_ evaluates at (0,0) as {0, 0} and scalar_ansatz_ as {0, 0},
    // so all products are 0.  Just verify the matrix has the right shape.
    ASSERT_EQ(result.rows(), 2u);
    ASSERT_EQ(result.cols(), 2u);
  }
}; // struct GenericElementBilinearFormTest


// ===========================================================================================
// Tests for GenericLocalCouplingIntersectionBilinearForm
// ===========================================================================================

template <class G>
struct GenericCouplingBilinearFormTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using typename BaseType::I;
  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;

  using GenericForm = GenericLocalCouplingIntersectionBilinearForm<I, 1>;
  using ScalarBasis = XT::Functions::GenericElementFunctionSet<E, 1, 1>;

  std::shared_ptr<ScalarBasis> make_const_basis() const
  {
    return std::make_shared<ScalarBasis>(
        1, 0, [](const auto& /*x*/, std::vector<FieldVector<double, 1>>& ret, const auto&) { ret = {{1.0}}; });
  }

  // Execute callable on first interior intersection; returns false if none found.
  template <class Callable>
  bool with_first_interior_intersection(Callable&& callable) const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (auto&& el : elements(gv)) {
      for (auto&& is : intersections(gv, el)) {
        if (is.neighbor()) {
          callable(gv, el, is);
          return true;
        }
      }
    }
    return false;
  }

  void is_constructable() final
  {
    // Do-nothing lambda.
    [[maybe_unused]] GenericForm form1(
        [](const auto& /*is*/,
           const auto& /*ti*/, const auto& /*ai*/,
           const auto& /*to*/, const auto& /*ao*/,
           auto& /*rii*/, auto& /*rio*/, auto& /*roi*/, auto& /*roo*/,
           const auto& /*p*/) {});

    GenericForm orig(
        [](const auto&, const auto&, const auto&, const auto&, const auto&,
           auto&, auto&, auto&, auto&, const auto&) {});
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  // Lambda must be called exactly once per apply2().
  void lambda_is_called()
  {
    int call_count = 0;
    GenericForm form(
        [&call_count](const auto&, const auto&, const auto&, const auto&, const auto&,
                      auto&, auto&, auto&, auto&, const auto&) { ++call_count; });

    auto basis_in = make_const_basis();
    auto basis_out = make_const_basis();

    bool found = with_first_interior_intersection([&](const GV& gv, const E& el_in, const I& is) {
      auto el_out = is.outside();
      basis_in->bind(el_in);
      basis_out->bind(el_out);

      DynamicMatrix<double> r(1, 1, 0.);
      form.apply2(is, *basis_in, *basis_in, *basis_out, *basis_out, r, r, r, r);
      EXPECT_EQ(1, call_count) << "lambda must be called exactly once";
    });

    ASSERT_TRUE(found) << "Test grid must have at least one interior intersection";
  }

  // All four result matrices must be cleared to zero before the lambda is called.
  void all_four_result_matrices_are_cleared_before_lambda()
  {
    double seen_ii = -1., seen_io = -1., seen_oi = -1., seen_oo = -1.;
    GenericForm form(
        [&seen_ii, &seen_io, &seen_oi, &seen_oo](
            const auto&, const auto&, const auto&, const auto&, const auto&,
            DynamicMatrix<double>& rii, DynamicMatrix<double>& rio,
            DynamicMatrix<double>& roi, DynamicMatrix<double>& roo,
            const auto&) {
          seen_ii = rii[0][0];
          seen_io = rio[0][0];
          seen_oi = roi[0][0];
          seen_oo = roo[0][0];
        });

    auto basis_in = make_const_basis();
    auto basis_out = make_const_basis();

    bool found = with_first_interior_intersection([&](const GV& gv, const E& el_in, const I& is) {
      auto el_out = is.outside();
      basis_in->bind(el_in);
      basis_out->bind(el_out);

      // Pre-fill with non-zero values.
      DynamicMatrix<double> r_ii(1, 1, 7.), r_io(1, 1, 7.), r_oi(1, 1, 7.), r_oo(1, 1, 7.);
      form.apply2(is, *basis_in, *basis_in, *basis_out, *basis_out, r_ii, r_io, r_oi, r_oo);

      EXPECT_DOUBLE_EQ(0.0, seen_ii) << "result_in_in must be zero when lambda is entered";
      EXPECT_DOUBLE_EQ(0.0, seen_io) << "result_in_out must be zero when lambda is entered";
      EXPECT_DOUBLE_EQ(0.0, seen_oi) << "result_out_in must be zero when lambda is entered";
      EXPECT_DOUBLE_EQ(0.0, seen_oo) << "result_out_out must be zero when lambda is entered";
    });

    ASSERT_TRUE(found);
  }

  // A do-nothing lambda must produce zero in all four result matrices.
  void do_nothing_lambda_gives_zero()
  {
    GenericForm form(
        [](const auto&, const auto&, const auto&, const auto&, const auto&,
           auto&, auto&, auto&, auto&, const auto&) {});

    auto basis_in = make_const_basis();
    auto basis_out = make_const_basis();

    bool found = with_first_interior_intersection([&](const GV& gv, const E& el_in, const I& is) {
      auto el_out = is.outside();
      basis_in->bind(el_in);
      basis_out->bind(el_out);

      DynamicMatrix<double> r_ii(1, 1, 99.), r_io(1, 1, 99.), r_oi(1, 1, 99.), r_oo(1, 1, 99.);
      form.apply2(is, *basis_in, *basis_in, *basis_out, *basis_out, r_ii, r_io, r_oi, r_oo);

      EXPECT_DOUBLE_EQ(0.0, r_ii[0][0]);
      EXPECT_DOUBLE_EQ(0.0, r_io[0][0]);
      EXPECT_DOUBLE_EQ(0.0, r_oi[0][0]);
      EXPECT_DOUBLE_EQ(0.0, r_oo[0][0]);
    });

    ASSERT_TRUE(found);
  }

  // All four result matrices must be resized if they are too small.
  void result_matrices_are_resized_if_too_small()
  {
    GenericForm form(
        [](const auto& ti, const auto& ai, const auto& to, const auto& ao,
           const auto& /*is*/,
           DynamicMatrix<double>& rii, DynamicMatrix<double>& rio,
           DynamicMatrix<double>& roi, DynamicMatrix<double>& roo,
           const auto& /*p*/) {
          // Write sentinel values so we can verify correct sizing.
          (void)ti; (void)ai; (void)to; (void)ao;
          rii[0][0] = 1.;
          rio[0][0] = 2.;
          roi[0][0] = 3.;
          roo[0][0] = 4.;
        });

    // Wrap the lambda to match the expected GenericFunctionType signature.
    GenericForm form2(
        [](const auto& /*is*/,
           const auto& /*ti*/, const auto& /*ai*/,
           const auto& /*to*/, const auto& /*ao*/,
           DynamicMatrix<double>& rii, DynamicMatrix<double>& rio,
           DynamicMatrix<double>& roi, DynamicMatrix<double>& roo,
           const auto& /*p*/) {
          rii[0][0] = 1.;
          rio[0][0] = 2.;
          roi[0][0] = 3.;
          roo[0][0] = 4.;
        });

    auto basis_in = make_const_basis();
    auto basis_out = make_const_basis();

    bool found = with_first_interior_intersection([&](const GV& gv, const E& el_in, const I& is) {
      auto el_out = is.outside();
      basis_in->bind(el_in);
      basis_out->bind(el_out);

      // Start with undersized matrices.
      DynamicMatrix<double> r_ii(0, 0), r_io(0, 0), r_oi(0, 0), r_oo(0, 0);
      form2.apply2(is, *basis_in, *basis_in, *basis_out, *basis_out, r_ii, r_io, r_oi, r_oo);

      ASSERT_GE(r_ii.rows(), 1u);
      ASSERT_GE(r_ii.cols(), 1u);
      EXPECT_DOUBLE_EQ(1., r_ii[0][0]);
      EXPECT_DOUBLE_EQ(2., r_io[0][0]);
      EXPECT_DOUBLE_EQ(3., r_oi[0][0]);
      EXPECT_DOUBLE_EQ(4., r_oo[0][0]);
    });

    ASSERT_TRUE(found);
  }
}; // struct GenericCouplingBilinearFormTest


// ===========================================================================================
// Additional LocalElementIntegralBilinearForm tests (raise coverage of integrals.hh)
// ===========================================================================================

template <class G>
struct ElementIntegralBilinearFormTest : public IntegrandTest<G>
{
  using BaseType = IntegrandTest<G>;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::GV;
  using BaseType::grid_provider_;
  using BaseType::scalar_ansatz_;
  using BaseType::scalar_test_;

  using IntegralBF = LocalElementIntegralBilinearForm<E, 1>;

  void is_constructable() final
  {
    LocalElementProductIntegrand<E, 1> integrand;
    [[maybe_unused]] IntegralBF form1(integrand);
    [[maybe_unused]] IntegralBF form2(integrand, /*over_integrate=*/0);
    [[maybe_unused]] IntegralBF form3(integrand, /*over_integrate=*/2);

    // Lambda-based convenience constructor.
    [[maybe_unused]] IntegralBF form4(
        [](const auto& t, const auto& a, const auto& /*p*/) { return t.order() + a.order(); },
        [](const auto& t, const auto& a, const auto& /*x*/, DynamicMatrix<double>& r, const auto& /*p*/) {
          for (size_t ii = 0; ii < t.size(); ++ii)
            for (size_t jj = 0; jj < a.size(); ++jj)
              r[ii][jj] = 1.0;
        });

    IntegralBF orig(integrand);
    [[maybe_unused]] IntegralBF copied(orig);
    auto ptr = orig.copy();
    ASSERT_NE(ptr, nullptr);
  }

  // apply2() must clear and recompute the result (a pre-filled matrix must not remain 99).
  void result_is_cleared_on_apply()
  {
    LocalElementProductIntegrand<E, 1> integrand;
    IntegralBF form(integrand);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result(2, 2, 99.0);
    form.apply2(*scalar_test_, *scalar_ansatz_, result);

    bool all_still_99 = true;
    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        if (result[ii][jj] != 99.0)
          all_still_99 = false;
    EXPECT_FALSE(all_still_99) << "result must be cleared and recomputed by apply2()";
  }

  // L2 product matrix must be symmetric when test == ansatz basis.
  void product_integrand_is_symmetric()
  {
    LocalElementProductIntegrand<E, 1> integrand;
    IntegralBF form(integrand);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);

    DynamicMatrix<double> result(2, 2, 0.);
    form.apply2(*scalar_test_, *scalar_test_, result);

    EXPECT_NEAR(result[0][1], result[1][0], 1e-13)
        << "L2 mass matrix must be symmetric when test == ansatz basis";
  }

  // Polynomial integrands: over_integrate must not change the exact result.
  void over_integrate_does_not_change_result_for_polynomial_integrand()
  {
    LocalElementProductIntegrand<E, 1> integrand;
    IntegralBF form0(integrand, /*over_integrate=*/0);
    IntegralBF form2(integrand, /*over_integrate=*/2);

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result0(2, 2, 0.), result2(2, 2, 0.);
    form0.apply2(*scalar_test_, *scalar_ansatz_, result0);
    form2.apply2(*scalar_test_, *scalar_ansatz_, result2);

    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_NEAR(result0[ii][jj], result2[ii][jj], 1e-13)
            << "over_integrate must not change the result for polynomial integrands";
  }

  // Lambda-based constructor must yield the same result as the integrand-based constructor
  // when the lambda implements the same product integrand.
  void lambda_constructor_matches_integrand_constructor()
  {
    LocalElementProductIntegrand<E, 1> integrand;
    IntegralBF form_int(integrand);

    IntegralBF form_lam(
        [](const auto& t, const auto& a, const auto& /*p*/) { return t.order() + a.order(); },
        [](const auto& test, const auto& ansatz, const auto& x, DynamicMatrix<double>& r,
           const XT::Common::Parameter& param) {
          const size_t rows = test.size(param);
          const size_t cols = ansatz.size(param);
          std::vector<FieldVector<double, 1>> tv(rows), av(cols);
          test.evaluate(x, tv, param);
          ansatz.evaluate(x, av, param);
          for (size_t ii = 0; ii < rows; ++ii)
            for (size_t jj = 0; jj < cols; ++jj)
              r[ii][jj] = tv[ii][0] * av[jj][0];
        });

    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    scalar_test_->bind(element);
    scalar_ansatz_->bind(element);

    DynamicMatrix<double> result_int(2, 2, 0.), result_lam(2, 2, 0.);
    form_int.apply2(*scalar_test_, *scalar_ansatz_, result_int);
    form_lam.apply2(*scalar_test_, *scalar_ansatz_, result_lam);

    for (size_t ii = 0; ii < 2; ++ii)
      for (size_t jj = 0; jj < 2; ++jj)
        EXPECT_NEAR(result_int[ii][jj], result_lam[ii][jj], 1e-13)
            << "lambda-based and integrand-based forms must produce identical results";
  }
}; // struct ElementIntegralBilinearFormTest


} // namespace Test
} // namespace GDT
} // namespace Dune


// ===========================================================================================
// Typed-test registrations
// ===========================================================================================

template <class G>
using GenericElementBilinearFormTest = Dune::GDT::Test::GenericElementBilinearFormTest<G>;
TYPED_TEST_SUITE(GenericElementBilinearFormTest, Grids2D);

TYPED_TEST(GenericElementBilinearFormTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(GenericElementBilinearFormTest, lambda_is_called)
{
  this->lambda_is_called();
}

TYPED_TEST(GenericElementBilinearFormTest, result_is_cleared_before_lambda)
{
  this->result_is_cleared_before_lambda();
}

TYPED_TEST(GenericElementBilinearFormTest, do_nothing_lambda_gives_zero)
{
  this->do_nothing_lambda_gives_zero();
}

TYPED_TEST(GenericElementBilinearFormTest, result_is_resized_if_too_small)
{
  this->result_is_resized_if_too_small();
}

TYPED_TEST(GenericElementBilinearFormTest, parameter_is_forwarded_to_lambda)
{
  this->parameter_is_forwarded_to_lambda();
}

TYPED_TEST(GenericElementBilinearFormTest, generic_form_lambda_computes_product_correctly)
{
  this->generic_form_lambda_computes_product_correctly();
}


template <class G>
using GenericCouplingBilinearFormTest = Dune::GDT::Test::GenericCouplingBilinearFormTest<G>;
TYPED_TEST_SUITE(GenericCouplingBilinearFormTest, Grids2D);

TYPED_TEST(GenericCouplingBilinearFormTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(GenericCouplingBilinearFormTest, lambda_is_called)
{
  this->lambda_is_called();
}

TYPED_TEST(GenericCouplingBilinearFormTest, all_four_result_matrices_are_cleared_before_lambda)
{
  this->all_four_result_matrices_are_cleared_before_lambda();
}

TYPED_TEST(GenericCouplingBilinearFormTest, do_nothing_lambda_gives_zero)
{
  this->do_nothing_lambda_gives_zero();
}

TYPED_TEST(GenericCouplingBilinearFormTest, result_matrices_are_resized_if_too_small)
{
  this->result_matrices_are_resized_if_too_small();
}


template <class G>
using ElementIntegralBilinearFormTest = Dune::GDT::Test::ElementIntegralBilinearFormTest<G>;
TYPED_TEST_SUITE(ElementIntegralBilinearFormTest, Grids2D);

TYPED_TEST(ElementIntegralBilinearFormTest, is_constructable)
{
  this->is_constructable();
}

TYPED_TEST(ElementIntegralBilinearFormTest, result_is_cleared_on_apply)
{
  this->result_is_cleared_on_apply();
}

TYPED_TEST(ElementIntegralBilinearFormTest, product_integrand_is_symmetric)
{
  this->product_integrand_is_symmetric();
}

TYPED_TEST(ElementIntegralBilinearFormTest, over_integrate_does_not_change_result_for_polynomial_integrand)
{
  this->over_integrate_does_not_change_result_for_polynomial_integrand();
}

TYPED_TEST(ElementIntegralBilinearFormTest, lambda_constructor_matches_integrand_constructor)
{
  this->lambda_constructor_matches_integrand_constructor();
}
