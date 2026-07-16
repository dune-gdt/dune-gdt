// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/functions/interfaces/function.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>

#include <dune/gdt/tools/hyperbolic.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

// Only the scalar (m == 1) case is bound (see numerical-fluxes_for_all_grids.hh for the WP6 scoping
// rationale): dt for a scalar conservation law u_t + div(f(u)) = 0 is estimated from the state's
// data range, the flux derivative on that range and the grid's worst perimeter/volume ratio,
// following [Cockburn, Coquel, LeFloch, 1995]. The function itself is tiny, so (unlike the heavy
// operator bindings) every grid of every dimension fits into this single translation unit; the
// per-grid overloads accumulate under the one factory name and are disambiguated by the grid
// provider argument.

template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct EstimateDtForHyperbolicSystem_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  using GP = Dune::XT::Grid::GridProvider<G>;
  using E = Dune::XT::Grid::extract_entity_t<GV>;
  static const constexpr size_t d = G::dimension;
  using StateType = Dune::XT::Functions::GridFunctionInterface<E, 1, 1, double>;
  using FluxType = Dune::XT::Functions::FunctionInterface<1, d, 1, double>;

  static void bind(pybind11::module& m)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    m.def(
        "estimate_dt_for_hyperbolic_system",
        [](GP& grid, const StateType& state, const FluxType& flux) {
          return Dune::GDT::estimate_dt_for_hyperbolic_system(grid.leaf_view(), state, flux);
        },
        "grid"_a,
        "state"_a,
        "flux"_a,
        py::call_guard<py::gil_scoped_release>());

    EstimateDtForHyperbolicSystem_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct EstimateDtForHyperbolicSystem_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {} // recursion base case: no grid types left to bind
};


PYBIND11_MODULE(_tools_hyperbolic, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  EstimateDtForHyperbolicSystem_for_all_grids<>::bind(m);
}
