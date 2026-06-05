// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
//
// Standalone benchmark (no gtest / no test harness) of the 1d Burgers equation solved with an
// explicit-Euler finite-volume scheme and an upwind numerical flux. Mirrors the computation in
// dune/gdt/test/burgers/ but reuses only the shared, gtest-free problem data.

#include "config.h"

#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/parallel/threadmanager.hh>
#include <dune/xt/grid/filters/intersection.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/view/periodic.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/data/burgers.hh>
#include <dune/gdt/local/numerical-fluxes/upwind.hh>
#include <dune/gdt/operators/advection-fv.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/tools/hyperbolic.hh>

#include "benchmark_common.hh"

using namespace Dune;
using namespace Dune::GDT;

int main(int argc, char** argv)
{
  MPIHelper::instance(argc, argv);
  XT::Common::threadManager().set_max_threads(1);

  using G = YASP_1D_EQUIDISTANT_OFFSET;
  static constexpr size_t d = G::dimension;
  static constexpr size_t m = 1;
  using R = double;
  using M = XT::LA::IstlRowMajorSparseMatrix<R>;
  using V = XT::LA::IstlDenseVector<R>;

  const Data::BurgersProblem<G> problem;
  auto grid = problem.make_initial_grid();
  grid.global_refine(6); // ~16 * 2^6 = 1024 cells, enough work for a meaningful measurement

  auto periodic_grid_view = XT::Grid::make_periodic_grid_layer(grid.leaf_view());
  using GV = decltype(periodic_grid_view);
  using I = XT::Grid::extract_intersection_t<GV>;

  const FiniteVolumeSpace<GV, m> space(periodic_grid_view);
  const auto u_0 = problem.template make_initial_values<V>(space);

  const NumericalUpwindFlux<I, d, m> numerical_flux(problem.flux);
  AdvectionFvOperator<GV, m, R, M> op(
      space.grid_view(), numerical_flux, space, space, XT::Grid::ApplyOn::NoIntersections<GV>());
  op.assemble();

  const double T_end = problem.T_end;
  const double dt = 0.99 * estimate_dt_for_hyperbolic_system(space.grid_view(), u_0, problem.flux);

  auto bench = Benchmark::make_bench("burgers__1d__explicit__fv");
  bench.run("explicit_euler__upwind", [&]() {
    auto u = u_0.dofs().vector();
    double time = 0.;
    while (time < T_end) {
      u -= op.apply(u, {{"_t", {time}}, {"_dt", {dt}}}) * dt;
      time += dt;
    }
    ankerl::nanobench::doNotOptimizeAway(u);
  });
  Benchmark::write_report(bench, "burgers__1d__explicit__fv");

  return 0;
}
