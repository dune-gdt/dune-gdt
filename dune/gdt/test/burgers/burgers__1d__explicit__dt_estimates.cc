// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

/**
 * \file burgers__1d__explicit__dt_estimates.cc
 * \brief Direct tests for the bisection based dt estimates of InstationaryNonconformingHyperbolicEocStudy.
 *
 * The EOC studies themselves never reach InstationaryNonconformingHyperbolicEocStudy::estimate_fixed_explicit_dt()
 * and ::estimate_fixed_explicit_dt_to_T_end(): the burgers .mini configs all pin setup.use_fixed_dt to a precomputed
 * positive value (the bisection is too costly to run in every EOC test), and the only other consumer,
 * inviscid-compressible-flow, guards both calls behind a non-fv space type it never instantiates. Both estimators are
 * therefore exercised here directly, on a single coarse grid with a small T_end and few steps, which is cheap enough
 * for CI while still checking what they promise: a dt within the searched bracket which keeps explicit Euler stable.
 */

#define DUNE_XT_COMMON_TEST_MAIN_ENABLE_TIMED_LOGGING 1
#define DUNE_XT_COMMON_TEST_MAIN_ENABLE_INFO_LOGGING 1

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/grid/grids.hh>

#include <dune/gdt/test/instationary-eocstudies/hyperbolic-nonconforming.hh>

#include "base.hh"

using namespace Dune;
using namespace Dune::GDT;


template <class G>
struct BurgersFixedExplicitDtTest : public BurgersExplicitTest<G>
{
  using BaseType = BurgersExplicitTest<G>;
  using typename BaseType::S;

  BurgersFixedExplicitDtTest()
    : BaseType()
  {
    this->numerical_flux_type_ = "engquist_osher";
  }

  /**
   * \brief Integrates the initial values with explicit Euler up to T_end and returns the resulting sup norm, relative
   *        to the one of the initial values.
   *
   * Anything below max_overshoot means the given dt was indeed stable in the sense of the estimators, which use the
   * very same criterion to accept a dt.
   */
  double relative_sup_norm_after_explicit_euler(const S& space, const double dt, const double T_end)
  {
    const auto u_0 = this->make_initial_values(space);
    const auto op = this->make_lhs_operator(space);
    auto u = u_0.dofs().vector();
    double time = 0.;
    while (time < T_end + dt) {
      u -= op->apply(u, {{"_t", {time}}, {"_dt", {dt}}}) * dt;
      time += dt;
    }
    return u.sup_norm() / u_0.dofs().vector().sup_norm();
  }
}; // struct BurgersFixedExplicitDtTest


using Burgers1dFixedExplicitDtTest = BurgersFixedExplicitDtTest<YASP_1D_EQUIDISTANT_OFFSET>;

TEST_F(Burgers1dFixedExplicitDtTest, estimate_fixed_explicit_dt_is_stable_for_dg)
{
  this->space_type_ = "dg_p1";
  const auto grid = this->make_initial_grid();
  const auto space = this->make_space(grid);
  // few steps only, we are merely after a dt which does not blow up immediately
  const double max_overshoot = 1.25;
  const int max_steps_to_try = 10;
  const auto dt = this->estimate_fixed_explicit_dt(*space, max_overshoot, max_steps_to_try);
  EXPECT_GT(dt, 0.);
  EXPECT_LE(dt, this->T_end_); // the bisection may not leave its bracket
  EXPECT_LE(this->relative_sup_norm_after_explicit_euler(*space, dt, /*T_end=*/max_steps_to_try * dt), max_overshoot);
}

TEST_F(Burgers1dFixedExplicitDtTest, estimate_fixed_explicit_dt_to_T_end_is_stable_for_dg)
{
  this->space_type_ = "dg_p1";
  const auto grid = this->make_initial_grid();
  const auto space = this->make_space(grid);
  // a fraction of the actual T_end of the problem keeps this cheap, the estimator does not care
  const double T_end = 0.1 * this->T_end_;
  const double min_dt = 1e-3;
  const double max_overshoot = 1.25;
  const auto dt = this->estimate_fixed_explicit_dt_to_T_end(*space, min_dt, T_end, max_overshoot);
  EXPECT_GE(dt, min_dt);
  EXPECT_LE(dt, T_end);
  EXPECT_LE(this->relative_sup_norm_after_explicit_euler(*space, dt, T_end), max_overshoot);
}

TEST_F(Burgers1dFixedExplicitDtTest, estimate_fixed_explicit_dt_to_T_end_is_stable_for_fv)
{
  this->space_type_ = "fv";
  const auto grid = this->make_initial_grid();
  const auto space = this->make_space(grid);
  const double T_end = 0.1 * this->T_end_;
  const double min_dt = 1e-3;
  const double max_overshoot = 1.25;
  const auto dt = this->estimate_fixed_explicit_dt_to_T_end(*space, min_dt, T_end, max_overshoot);
  EXPECT_GE(dt, min_dt);
  EXPECT_LE(dt, T_end);
  EXPECT_LE(this->relative_sup_norm_after_explicit_euler(*space, dt, T_end), max_overshoot);
}
