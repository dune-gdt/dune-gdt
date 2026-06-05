// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

#ifndef DUNE_GDT_DATA_BURGERS_HH
#define DUNE_GDT_DATA_BURGERS_HH

#include <cmath>

#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/interpolations/default.hh>
#include <dune/gdt/spaces/interface.hh>

namespace Dune {
namespace GDT {
namespace Data {


/**
 * \brief Problem data for the Burgers equation \partial_t u + \partial_x (u^2 / 2) = 0.
 *
 * gtest-free problem definition shared by the tests in dune/gdt/test/burgers/ and the
 * benchmarks in benchmarks/.
 */
template <class G>
struct BurgersProblem
{
  static constexpr size_t d = G::dimension;
  static_assert(d == 1, "Not implemented yet!");

  const XT::Functions::GenericFunction<1, d, 1> flux;
  const double T_end;

  BurgersProblem()
    : flux(
          2,
          [&](const auto& u, const auto& /*param*/) { return 0.5 * u * u; },
          "burgers",
          {},
          [&](const auto& u, const auto& /*param*/) { return u; })
    , T_end(1.)
  {
  }

  XT::Grid::GridProvider<G> make_initial_grid() const
  {
    return XT::Grid::make_cube_grid<G>(0., 1., 16u);
  }

  template <class Vector, class GV>
  DiscreteFunction<Vector, GV> make_initial_values(const SpaceInterface<GV, 1>& space) const
  {
    return default_interpolation<Vector>(
        3,
        [&](const auto& xx, const auto& /*mu*/) {
          return std::exp(-std::pow(xx[0] - 0.33, 2) / (2 * std::pow(0.075, 2)));
        },
        space);
  }
}; // struct BurgersProblem


} // namespace Data
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_DATA_BURGERS_HH
