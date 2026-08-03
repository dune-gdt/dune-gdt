// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <cmath>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/functions/constant.hh>
#include <dune/xt/functions/grid-function.hh>

#include <dune/gdt/tools/euler.hh>

using namespace Dune;
using namespace Dune::GDT;


// A physically meaningful conservative state w = (rho, rho*v, E) built from primitive variables with strictly positive
// density and pressure, so that the speed of sound a = sqrt(gamma * p / rho) (and thus the eigendecomposition) is real
// and well defined.
template <size_t d>
XT::Common::FieldVector<double, d + 2>
make_state(const EulerTools<d>& euler, const double rho, const XT::Common::FieldVector<double, d>& v, const double p)
{
  XT::Common::FieldVector<double, 1> rho_fv(rho);
  XT::Common::FieldVector<double, 1> p_fv(p);
  return euler.conservative(rho_fv, v, p_fv);
}


// primitives(conservative(rho, v, p)) reproduces (rho, v, p) exactly, and conservative(primitives(w)) reproduces w:
// the two conversions are inverse to each other.
template <size_t d>
void check_primitive_conservative_roundtrip()
{
  const EulerTools<d> euler(1.4);
  const double rho = 2.0;
  const double p = 1.5;
  XT::Common::FieldVector<double, d> v(0.);
  for (size_t ii = 0; ii < d; ++ii)
    v[ii] = 0.3 - 0.1 * ii;

  const auto w = make_state<d>(euler, rho, v, p);

  // conservative -> primitive
  const auto rho_v_p = euler.primitives(w);
  EXPECT_NEAR(rho, std::get<0>(rho_v_p)[0], 1e-13);
  for (size_t ii = 0; ii < d; ++ii)
    EXPECT_NEAR(v[ii], std::get<1>(rho_v_p)[ii], 1e-13);
  EXPECT_NEAR(p, std::get<2>(rho_v_p)[0], 1e-13);

  // the packed variant primitive(w) must agree with primitives(w)
  const auto packed = euler.primitive(w);
  EXPECT_NEAR(rho, packed[euler.density_index()], 1e-13);
  for (size_t ii = 0; ii < d; ++ii)
    EXPECT_NEAR(v[ii], packed[euler.velocity_indices()[ii]], 1e-13);
  EXPECT_NEAR(p, packed[euler.pressure_index()], 1e-13);

  // individual accessors
  EXPECT_NEAR(rho, euler.density(w)[0], 1e-13);
  for (size_t ii = 0; ii < d; ++ii)
    EXPECT_NEAR(v[ii], euler.velocity(w)[ii], 1e-13);
  EXPECT_NEAR(p, euler.pressure(w)[0], 1e-13);

  // primitive -> conservative: feeding the *extracted* primitives back through conservative(...) must reproduce w
  // (reconstructing from std::get<...>(rho_v_p), not from the original scalar inputs, actually exercises the inverse)
  const auto w2 = euler.conservative(std::get<0>(rho_v_p), std::get<1>(rho_v_p), std::get<2>(rho_v_p));
  for (size_t ii = 0; ii < d + 2; ++ii)
    EXPECT_NEAR(w[ii], w2[ii], 1e-13);
}


// The derived scalar quantities are consistent with their closed forms.
template <size_t d>
void check_derived_quantities()
{
  const EulerTools<d> euler(1.4);
  const double gamma = 1.4;
  const double rho = 1.3;
  const double p = 0.7;
  XT::Common::FieldVector<double, d> v(0.);
  for (size_t ii = 0; ii < d; ++ii)
    v[ii] = 0.2 * (ii + 1);
  const auto w = make_state<d>(euler, rho, v, p);

  // a^2 = gamma * p / rho, computed identically from w and from (rho, p)
  const double a2_expected = gamma * p / rho;
  EXPECT_NEAR(a2_expected, euler.speed_of_sound2(w), 1e-13);
  EXPECT_NEAR(std::sqrt(a2_expected), euler.speed_of_sound(w), 1e-13);
  XT::Common::FieldVector<double, 1> rho_fv(rho);
  XT::Common::FieldVector<double, 1> p_fv(p);
  EXPECT_NEAR(a2_expected, euler.speed_of_sound2(rho_fv, p_fv), 1e-13);

  // Mach number M = |v| / a
  EXPECT_NEAR(v.two_norm() / std::sqrt(a2_expected), euler.mach_number(w), 1e-13);
  EXPECT_NEAR(v.two_norm() / std::sqrt(a2_expected), euler.mach_number(rho_fv, v, p_fv), 1e-13);

  // enthalpy H = (E + p) / rho
  const auto E = euler.energy(w);
  EXPECT_NEAR((E[0] + p) / rho, euler.enthalpy(w), 1e-13);
}


// The Euler flux at v = 0: mass flux vanishes, the momentum flux is the pressure on the diagonal and the energy flux
// vanishes (a simple hand-verifiable special case that pins flux()).
template <size_t d>
void check_flux_at_rest()
{
  const EulerTools<d> euler(1.4);
  const double rho = 1.1;
  const double p = 0.9;
  XT::Common::FieldVector<double, d> v(0.);
  const auto w = make_state<d>(euler, rho, v, p);

  const auto f = euler.flux(w); // f is a (d x m) matrix, row s is the flux in direction s
  for (size_t s = 0; s < d; ++s) {
    EXPECT_NEAR(0., f[s][euler.density_index()], 1e-13);
    for (size_t ii = 0; ii < d; ++ii)
      EXPECT_NEAR((s == ii) ? p : 0., f[s][euler.velocity_indices()[ii]], 1e-13);
    EXPECT_NEAR(0., f[s][euler.energy_index()], 1e-13);
  }

  // at an impermeable wall (v.n = 0) only the pressure contributes to the momentum components
  XT::Common::FieldVector<double, d> n(0.);
  n[0] = 1.;
  const auto wall_flux = euler.flux_at_impermeable_walls(w, n);
  EXPECT_NEAR(0., wall_flux[euler.density_index()], 1e-13);
  EXPECT_NEAR(0., wall_flux[euler.energy_index()], 1e-13);
  for (size_t ii = 0; ii < d; ++ii)
    EXPECT_NEAR((ii == 0) ? p : 0., wall_flux[euler.velocity_indices()[ii]], 1e-13);
}


// The eigendecomposition of P = sum_s n_s A_s (the flux jacobian contracted with a unit normal n) must satisfy
// P T = T Lambda (i.e. P t_i = lambda_i t_i for every eigenvector column t_i) and T^{-1} T = I. This exercises
// flux_jacobian(), eigenvalues_flux_jacobian(), eigenvectors_flux_jacobian() and eigenvectors_inv_flux_jacobian()
// together and is only defined for d in {1, 2} (higher dimensions throw NotImplemented, see below).
template <size_t d>
void check_eigendecomposition(const XT::Common::FieldVector<double, d>& n)
{
  static constexpr size_t m = d + 2;
  const EulerTools<d> euler(1.4);
  const double rho = 1.4;
  const double p = 1.2;
  XT::Common::FieldVector<double, d> v(0.);
  for (size_t ii = 0; ii < d; ++ii)
    v[ii] = 0.15 * (ii + 1);
  const auto w = make_state<d>(euler, rho, v, p);

  // P = sum_s n_s A_s
  const auto jacobian = euler.flux_jacobian(w);
  XT::Common::FieldMatrix<double, m, m> P(0.);
  for (size_t s = 0; s < d; ++s)
    for (size_t row = 0; row < m; ++row)
      for (size_t col = 0; col < m; ++col)
        P[row][col] += n[s] * jacobian[s][row][col];

  const auto evs = euler.eigenvalues_flux_jacobian(w, n);
  const auto T = euler.eigenvectors_flux_jacobian(w, n);
  const auto T_inv = euler.eigenvectors_inv_flux_jacobian(w, n);

  // P t_i = lambda_i t_i for the i-th eigenvector (i-th column of T)
  for (size_t i = 0; i < m; ++i) {
    XT::Common::FieldVector<double, m> t_i(0.);
    for (size_t row = 0; row < m; ++row)
      t_i[row] = T[row][i];
    XT::Common::FieldVector<double, m> Pt(0.);
    P.mv(t_i, Pt);
    for (size_t row = 0; row < m; ++row)
      EXPECT_NEAR(evs[i] * t_i[row], Pt[row], 1e-10);
  }

  // the inverse eigenvector matrix left-multiplied by the eigenvector matrix must yield the identity
  for (size_t i = 0; i < m; ++i) {
    for (size_t j = 0; j < m; ++j) {
      double entry = 0.;
      for (size_t k = 0; k < m; ++k)
        entry += T_inv[i][k] * T[k][j];
      EXPECT_NEAR((i == j) ? 1. : 0., entry, 1e-10);
    }
  }
}


TEST(euler_tools, primitive_conservative_roundtrip)
{
  check_primitive_conservative_roundtrip<1>();
  check_primitive_conservative_roundtrip<2>();
  check_primitive_conservative_roundtrip<3>();
}

TEST(euler_tools, derived_quantities)
{
  check_derived_quantities<1>();
  check_derived_quantities<2>();
  check_derived_quantities<3>();
}

TEST(euler_tools, flux_at_rest_and_impermeable_walls)
{
  check_flux_at_rest<1>();
  check_flux_at_rest<2>();
  check_flux_at_rest<3>();
}

TEST(euler_tools, eigendecomposition_1d)
{
  XT::Common::FieldVector<double, 1> n(1.);
  check_eigendecomposition<1>(n);
}

TEST(euler_tools, eigendecomposition_2d)
{
  // axis-aligned and a genuinely oblique unit normal
  XT::Common::FieldVector<double, 2> n_x{1., 0.};
  XT::Common::FieldVector<double, 2> n_y{0., 1.};
  XT::Common::FieldVector<double, 2> n_oblique{0.6, 0.8};
  check_eigendecomposition<2>(n_x);
  check_eigendecomposition<2>(n_y);
  check_eigendecomposition<2>(n_oblique);
}

TEST(euler_tools, jacobian_and_eigendecomposition_not_implemented_in_3d)
{
  const EulerTools<3> euler(1.4);
  XT::Common::FieldVector<double, 3> v{0.1, 0.2, 0.3};
  const auto w = make_state<3>(euler, 1.2, v, 0.8);
  XT::Common::FieldVector<double, 3> n{1., 0., 0.};
  EXPECT_THROW(euler.flux_jacobian(w), Dune::NotImplemented);
  EXPECT_THROW(euler.eigenvalues_flux_jacobian(w, n), Dune::NotImplemented);
  EXPECT_THROW(euler.eigenvectors_flux_jacobian(w, n), Dune::NotImplemented);
  EXPECT_THROW(euler.eigenvectors_inv_flux_jacobian(w, n), Dune::NotImplemented);
}

TEST(euler_tools, visualize_conservative_state)
{
  static constexpr size_t d = 2;
  static constexpr size_t m = d + 2;
  using G = YASP_2D_EQUIDISTANT_OFFSET;
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;

  const EulerTools<d> euler(1.4);
  auto grid = XT::Grid::make_cube_grid<G>(/*lower_left=*/0., /*upper_right=*/1., /*num_elements=*/2);
  auto grid_view = grid.leaf_view();

  // a constant, physically valid conservative state w = (rho, rho*v, E)
  XT::Common::FieldVector<double, d> v{0.1, -0.2};
  const auto w0 = make_state<d>(euler, 1.0, v, 1.0);
  XT::Functions::ConstantFunction<d, m, 1, double> constant_w(w0);
  const auto u_conservative = XT::Functions::make_grid_function<E>(constant_w);

  // just assert the visualization (density, momentum, energy, velocity, pressure) runs without throwing
  EXPECT_NO_THROW(euler.visualize(u_conservative, grid_view, "tools_euler_test", "0"));
}
