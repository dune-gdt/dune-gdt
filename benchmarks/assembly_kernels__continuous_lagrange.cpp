// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
//
// Microbenchmark of the element-local assembly kernels in isolation: a continuous-Lagrange H1
// space on a structured (YaspGrid) cube grid, assembling a stiffness (LocalLaplaceIntegrand) and
// a mass (LocalElementProductIntegrand) bilinear form. There is no DG coupling and no linear
// solve, so the measurement is dominated by the per-element quadrature loop, basis evaluation and
// local-to-global scatter -- the hot path targeted by the assembly-density work (see issue #151).
//
// The grid is affine (cubes), the case that the affine fast-path and precomputed shape-function
// table work is meant to accelerate, so this benchmark is the primary before/after yardstick.
//
// FE order (1/2/3) and grid refinement are swept to expose how the kernels scale with the local
// matrix size and the number of quadrature points. YaspGrid is always available, so this benchmark
// needs no optional grid module.

#include "config.h"

#include <string>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/parallel/threadmanager.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/walker.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/integrands/laplace.hh>
#include <dune/gdt/local/integrands/product.hh>
#include <dune/gdt/operators/bilinear-form.hh>
#include <dune/gdt/operators/matrix.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

#include "benchmark_common.hh"

using namespace Dune;
using namespace Dune::GDT;

namespace {


// Assemble a single element bilinear form (no intersection terms, no solve) into a sparse matrix.
// The assembled matrix norm is fed to doNotOptimizeAway so the optimizer cannot elide the work.
template <class GV, class Integrand>
void assemble_element_form(const GV& grid_view, const int order, Integrand integrand)
{
  using E = XT::Grid::extract_entity_t<GV>;
  using M = XT::LA::IstlRowMajorSparseMatrix<double>;

  const ContinuousLagrangeSpace<GV> space(grid_view, order);
  auto form = make_bilinear_form(grid_view);
  form += LocalElementIntegralBilinearForm<E>(std::move(integrand));
  auto matrix_op = make_matrix_operator<M>(space);
  matrix_op.append(form);
  auto walker = XT::Grid::make_walker(grid_view);
  walker.append(matrix_op);
  walker.walk(/*parallel=*/false);
  ankerl::nanobench::doNotOptimizeAway(matrix_op.matrix().sup_norm());
}


template <class G>
void run_for_grid(ankerl::nanobench::Bench& bench,
                  const std::string& dim_tag,
                  const unsigned int elements_per_dim,
                  const std::vector<int>& orders)
{
  using GV = typename G::LeafGridView;
  using E = XT::Grid::extract_entity_t<GV>;

  auto grid = XT::Grid::make_cube_grid<G>(0., 1., elements_per_dim);
  const GV grid_view = grid.leaf_view();

  for (const int order : orders) {
    bench.run(dim_tag + "__laplace__p" + std::to_string(order),
              [&]() { assemble_element_form(grid_view, order, LocalLaplaceIntegrand<E>()); });
    bench.run(dim_tag + "__product__p" + std::to_string(order),
              [&]() { assemble_element_form(grid_view, order, LocalElementProductIntegrand<E>()); });
  }
}


} // namespace


int main(int argc, char** argv)
{
  MPIHelper::instance(argc, argv);
  XT::Common::threadManager().set_max_threads(1);

  auto bench = Benchmark::make_bench("assembly_kernels__continuous_lagrange");
  // higher epoch counts than the heavy e2e benchmarks: a single element-assembly sweep is cheap
  bench.warmup(1).epochs(5).minEpochIterations(1);

  // 2D: 128x128 cells; 3D: 24x24x24 cells -- comparable element counts, kept modest so a sweep
  // over orders finishes quickly while still amortizing per-walk overhead.
  run_for_grid<YASP_2D_EQUIDISTANT_OFFSET>(bench, "2d", 128u, {1, 2, 3});
  run_for_grid<YASP_3D_EQUIDISTANT_OFFSET>(bench, "3d", 24u, {1, 2, 3});

  Benchmark::write_report(bench, "assembly_kernels__continuous_lagrange");
  return 0;
}
