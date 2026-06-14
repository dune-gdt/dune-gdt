// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

/**
 * \file  stationary_heat_equation.hh
 * \brief Problem data for the stationary heat equation test case.
 **/
#ifndef DUNE_GDT_DATA_STATIONARY_HEAT_EQUATION_HH
#define DUNE_GDT_DATA_STATIONARY_HEAT_EQUATION_HH

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/type_traits.hh>
#include <dune/xt/la/container/eye-matrix.hh>
#include <dune/xt/grid/boundaryinfo/alldirichlet.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/functions/ESV2007.hh>
#include <dune/xt/functions/constant.hh>
#include <dune/xt/functions/grid-function.hh>

namespace Dune {
namespace GDT {
namespace Data {


/**
 * \brief Diffusion problem data from [1], Section 8.1 (stationary heat equation).
 *
 * gtest-free problem definition shared by the tests in
 * dune/gdt/test/stationary-heat-equation/ and the benchmarks in benchmarks/.
 *
 * [1]: Ern, Stephansen, Vohralik, 2007, Improved energy norm a posteriori error estimation based on flux reconstruction
 *      for discontinuous Galerkin methods, Preprint R07050, Laboratoire Jacques-Louis Lions & HAL Preprint
 */
template <class GV>
struct ESV2007DiffusionProblem
{
  static_assert(XT::Grid::is_view<GV>::value, "");
  static_assert(GV::dimension == 2, "");

  static constexpr size_t d = GV::dimension;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using G = typename GV::Grid;

  // We can only reproduce the results from ESV2007 by using a quadrature of order 3, which we obtain with a p1 DG space
  // and a force of order 2.
  explicit ESV2007DiffusionProblem(int force_order = 2)
    : diffusion(XT::LA::eye_matrix<XT::Common::FieldMatrix<double, d, d>>(d, d))
    , dirichlet(0.)
    , neumann(0.)
    , force(XT::Functions::ESV2007::Testcase1Force<d, 1>(force_order))
  {
  }

  XT::Grid::GridProvider<G> make_initial_grid() const
  {
    if (std::is_same<G, YASP_2D_EQUIDISTANT_OFFSET>::value) {
      return XT::Grid::make_cube_grid<G>(-1, 1, 8);
#if HAVE_DUNE_ALUGRID
    } else if (std::is_same<G, ALU_2D_SIMPLEX_CONFORMING>::value) {
      auto grid = XT::Grid::make_cube_grid<G>(-1, 1, 4);
      grid.global_refine(2);
      return grid;
    } else if (std::is_same<G, ALU_2D_SIMPLEX_NONCONFORMING>::value) {
      return XT::Grid::make_cube_grid<G>(-1, 1, 8);
    } else if (std::is_same<G, ALU_2D_CUBE>::value) {
      auto grid = XT::Grid::make_cube_grid<G>(-1, 1, 8);
      grid.global_refine(1);
      return grid;
#endif // HAVE_DUNE_ALUGRID
    } else
      DUNE_THROW(XT::Common::Exceptions::wrong_input_given,
                 "Please add a specialization for '" << XT::Common::Typename<G>::value() << "'!");
  } // ... make_initial_grid(...)

  const XT::Functions::GridFunction<E, d, d> diffusion;
  const XT::Functions::GridFunction<E> dirichlet;
  const XT::Functions::GridFunction<E> neumann;
  const XT::Functions::GridFunction<E> force;
  const XT::Grid::AllDirichletBoundaryInfo<I> boundary_info;
}; // struct ESV2007DiffusionProblem


} // namespace Data
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_DATA_STATIONARY_HEAT_EQUATION_HH
