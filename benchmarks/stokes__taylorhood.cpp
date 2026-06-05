// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
//
// Standalone benchmark (no gtest / no test harness) of a stationary Stokes problem discretized
// with Taylor-Hood (P2/Q2 velocity, P1/Q1 pressure) elements and solved with a direct saddle-point
// solver. Mirrors the assemble+solve in dune/gdt/test/stokes/ but reuses only the shared,
// gtest-free problem data.

#include "config.h"

#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/parallel/threadmanager.hh>
#include <dune/xt/grid/boundaryinfo/types.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/walker.hh>
#include <dune/xt/functions/constant.hh>

#include "benchmark_common.hh"

#if HAVE_DUNE_ISTL

#  include <dune/xt/la/container/eigen.hh>
#  include <dune/xt/la/container/istl.hh>
#  include <dune/xt/la/solver/saddlepoint.hh>

#  include <dune/gdt/data/stokes.hh>
#  include <dune/gdt/discretefunction/default.hh>
#  include <dune/gdt/functionals/vector-based.hh>
#  include <dune/gdt/interpolations/boundary.hh>
#  include <dune/gdt/local/bilinear-forms/integrals.hh>
#  include <dune/gdt/local/functionals/integrals.hh>
#  include <dune/gdt/local/integrands/conversion.hh>
#  include <dune/gdt/local/integrands/div.hh>
#  include <dune/gdt/local/integrands/laplace.hh>
#  include <dune/gdt/local/integrands/product.hh>
#  include <dune/gdt/operators/bilinear-form.hh>
#  include <dune/gdt/operators/matrix.hh>
#  include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#  include <dune/gdt/tools/dirichlet-constraints.hh>

using namespace Dune;
using namespace Dune::GDT;

int main(int argc, char** argv)
{
  MPIHelper::instance(argc, argv);
  XT::Common::threadManager().set_max_threads(1);

  using G = YASP_2D_EQUIDISTANT_OFFSET;
  using GV = typename G::LeafGridView;
  static constexpr size_t d = G::dimension;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using RangeField = double;
  using Matrix = XT::LA::EigenRowMajorSparseMatrix<double>;
  using Vector = XT::LA::EigenDenseVector<double>;
  using VelocitySpace = ContinuousLagrangeSpace<GV, d>;
  using PressureSpace = ContinuousLagrangeSpace<GV, 1>;

  const int velocity_order = 2;

  Data::StokesDirichletProblem<GV> problem(Data::StokesDirichletProblem<GV>::default_diffusion(),
                                           Data::StokesDirichletProblem<GV>::default_rhs_f(),
                                           Data::StokesDirichletProblem<GV>::default_rhs_g(),
                                           Data::stokes_testcase1_dirichlet<GV>());
  const auto& grid_view = problem.grid_view();

  auto bench = Benchmark::make_bench("stokes__taylorhood");
  bench.run("taylor_hood_order2__assemble_and_solve", [&]() {
    const VelocitySpace velocity_space(grid_view, velocity_order);
    const PressureSpace pressure_space(grid_view, velocity_order - 1);
    // bilinear form a: \int \nabla u : \nabla v
    BilinearForm<GV, d> a(grid_view);
    a += LocalElementIntegralBilinearForm<E, d>(LocalLaplaceIntegrand<E, d>(problem.diffusion()));
    // bilinear form b: \int -p div(v)
    BilinearForm<GV, 1, 1, d, 1> b(grid_view);
    b += LocalElementIntegralBilinearForm<E, d, 1, RangeField, RangeField, 1>(
        LocalElementAnsatzValueTestDivProductIntegrand<E>(-1.));
    auto A = a.template with<Matrix>(velocity_space, velocity_space);
    auto B = b.template with<Matrix>(pressure_space, velocity_space);
    // rhs functional f
    auto f_functional = make_vector_functional<Vector>(velocity_space);
    f_functional.append(
        LocalElementIntegralFunctional<E, d>(LocalProductIntegrand<E, d>().with_ansatz(problem.rhs_f())));
    // integrated pressure basis (for fixing the pressure constant)
    auto p_basis_integrated_functional = make_vector_functional<Vector>(pressure_space);
    p_basis_integrated_functional.append(LocalElementIntegralFunctional<E, 1>(
        LocalProductIntegrand<E, 1>().with_ansatz(XT::Functions::ConstantGridFunction<E>(1))));
    // Dirichlet constraints for u
    DirichletConstraints<I, VelocitySpace> dirichlet_constraints(problem.boundary_info(), velocity_space);
    // assemble everything in one grid walk
    auto walker = XT::Grid::make_walker(grid_view);
    walker.append(A);
    walker.append(B);
    walker.append(f_functional);
    walker.append(p_basis_integrated_functional);
    walker.append(dirichlet_constraints);
    walker.walk(/*parallel=*/false);
    // build the right hand sides, eliminating the Dirichlet values
    const auto discrete_dirichlet_values = boundary_interpolation<Vector>(
        problem.dirichlet(), velocity_space, problem.boundary_info(), XT::Grid::DirichletBoundary());
    auto rhs_vector_u = A.apply(discrete_dirichlet_values.dofs().vector());
    rhs_vector_u *= -1.;
    rhs_vector_u += f_functional.vector();
    auto rhs_vector_p = B.matrix().mtv(discrete_dirichlet_values.dofs().vector());
    rhs_vector_p *= -1;
    dirichlet_constraints.apply(A.matrix(), rhs_vector_u);
    for (const auto& DoF : dirichlet_constraints.dirichlet_DoFs())
      B.matrix().clear_row(DoF);
    // fix the pressure at the first DoF to make the solution unique
    auto C = make_matrix_operator<Matrix>(pressure_space);
    const size_t dof_index = 0;
    B.matrix().clear_col(dof_index);
    rhs_vector_p.set_entry(dof_index, 0.);
    C.matrix().set_entry(dof_index, dof_index, 1.);
    // solve the saddle point system with a direct solver
    XT::LA::SaddlePointSolver<Vector, Matrix> solver(A.matrix(), B.matrix(), B.matrix(), C.matrix());
    auto solution_u = make_discrete_function<Vector>(velocity_space);
    auto solution_p = make_discrete_function<Vector>(pressure_space);
    solver.apply(rhs_vector_u, rhs_vector_p, solution_u.dofs().vector(), solution_p.dofs().vector(), "direct");
    ankerl::nanobench::doNotOptimizeAway(solution_u.dofs().vector());
    ankerl::nanobench::doNotOptimizeAway(solution_p.dofs().vector());
  });
  Benchmark::write_report(bench, "stokes__taylorhood");

  return 0;
}

#else // HAVE_DUNE_ISTL

int main()
{
  // The Stokes benchmark uses the dune-istl backed saddle-point solver; nothing to do otherwise.
  return 0;
}

#endif // HAVE_DUNE_ISTL
