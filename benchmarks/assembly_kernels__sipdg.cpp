// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
//
// Assembly-only microbenchmark of the SIPDG system for the stationary heat equation (ESV2007
// testcase). This is the assembly half of benchmarks/stationary_heat_equation__ESV2007.cpp with
// the linear solve removed, so the measurement isolates the element- and intersection-local
// assembly kernels (the hot path targeted by the assembly-density work, see issue #151) from the
// solver. It exercises the DG coupling/penalty integrands that the continuous-Lagrange element
// benchmark does not.
//
// Requires a simplex grid (dune-alugrid); a no-op otherwise, matching the e2e benchmark.

#include "config.h"

#include <functional>

#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/parallel/threadmanager.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/boundaryinfo/types.hh>
#include <dune/xt/grid/filters/intersection.hh>
#include <dune/xt/grid/walker.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/data/stationary_heat_equation.hh>
#include <dune/gdt/functionals/vector-based.hh>
#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/functionals/integrals.hh>
#include <dune/gdt/local/integrands/ipdg.hh>
#include <dune/gdt/local/integrands/laplace.hh>
#include <dune/gdt/local/integrands/laplace-ipdg.hh>
#include <dune/gdt/local/integrands/product.hh>
#include <dune/gdt/operators/bilinear-form.hh>
#include <dune/gdt/operators/matrix.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>

#include "benchmark_common.hh"

using namespace Dune;
using namespace Dune::GDT;

#if HAVE_DUNE_ALUGRID

int main(int argc, char** argv)
{
  MPIHelper::instance(argc, argv);
  XT::Common::threadManager().set_max_threads(1);

  using G = ALU_2D_SIMPLEX_CONFORMING;
  using GV = typename G::LeafGridView;
  static constexpr size_t d = G::dimension;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using M = XT::LA::IstlRowMajorSparseMatrix<double>;
  using V = XT::LA::IstlDenseVector<double>;

  // SIPDG parameters as used by the ESV2007 testcase
  const double inner_penalty = 8.;
  const double dirichlet_penalty = 14.;
  const std::function<double(const I&)> intersection_diameter = [](const I& intersection) {
    return intersection.geometry().volume();
  };

  const Data::ESV2007DiffusionProblem<GV> problem(/*force_order=*/2);
  auto grid = problem.make_initial_grid();
  grid.global_refine(2); // extra refinement on top of the testcase grid for a meaningful workload
  const GV grid_view = grid.leaf_view();

  const DiscontinuousLagrangeSpace<GV> space(grid_view, /*order=*/1);

  auto bench = Benchmark::make_bench("assembly_kernels__sipdg");
  bench.run("sipdg_p1__assemble_only", [&]() {
    // assemble the SIPDG bilinear form (weight == diffusion); no linear solve
    auto lhs = make_bilinear_form(space.grid_view());
    lhs += LocalElementIntegralBilinearForm<E>(LocalLaplaceIntegrand<E>(problem.diffusion));
    lhs += {LocalCouplingIntersectionIntegralBilinearForm<I>(
                LocalLaplaceIPDGIntegrands::InnerCoupling<I>(1., problem.diffusion, problem.diffusion)),
            XT::Grid::ApplyOn::InnerIntersectionsOnce<GV>()};
    lhs += {LocalCouplingIntersectionIntegralBilinearForm<I>(
                LocalIPDGIntegrands::InnerPenalty<I>(inner_penalty, problem.diffusion, intersection_diameter)),
            XT::Grid::ApplyOn::InnerIntersectionsOnce<GV>()};
    lhs +=
        {LocalIntersectionIntegralBilinearForm<I>(
             LocalLaplaceIPDGIntegrands::DirichletCoupling<I>(1., problem.diffusion)),
         XT::Grid::ApplyOn::CustomBoundaryIntersections<GV>(problem.boundary_info, new XT::Grid::DirichletBoundary())};
    lhs +=
        {LocalIntersectionIntegralBilinearForm<I>(
             LocalIPDGIntegrands::BoundaryPenalty<I>(dirichlet_penalty, problem.diffusion, intersection_diameter)),
         XT::Grid::ApplyOn::CustomBoundaryIntersections<GV>(problem.boundary_info, new XT::Grid::DirichletBoundary())};
    auto matrix_op = make_matrix_operator<M>(space);
    matrix_op.append(lhs);
    // assemble the right hand side functional
    auto rhs_func = make_vector_functional<V>(space);
    rhs_func.append(LocalElementIntegralFunctional<E>(LocalProductIntegrand<E>().with_ansatz(problem.force)));
    // assemble everything in one grid walk (no solve)
    auto walker = XT::Grid::make_walker(space.grid_view());
    walker.append(matrix_op);
    walker.append(rhs_func);
    walker.walk(/*parallel=*/false);
    ankerl::nanobench::doNotOptimizeAway(matrix_op.matrix().sup_norm());
    ankerl::nanobench::doNotOptimizeAway(rhs_func.vector().sup_norm());
  });
  Benchmark::write_report(bench, "assembly_kernels__sipdg");

  return 0;
}

#else // HAVE_DUNE_ALUGRID

int main()
{
  // The ESV2007 benchmark requires a simplex grid (dune-alugrid); nothing to do otherwise.
  return 0;
}

#endif // HAVE_DUNE_ALUGRID
