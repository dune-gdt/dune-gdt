// This file is part of the dune-xt project:
//   https://github.com/dune-community/dune-xt
// Copyright 2009-2020 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)
//   René Fritze     (2019)
//   Tobias Leibner  (2019 - 2020)

#include "config.h"

#include <string>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>

#include <dune/pybindxi/pybind11.h>
#include <dune/pybindxi/stl.h>

#include <python/dune/xt/common/bindings.hh>
#include <python/dune/xt/common/exceptions.bindings.hh>
#include <python/dune/xt/grid/grids.bindings.hh>

#include "spe10.hh"


template <class G>
void addbind_for_Grid(pybind11::module& m)
{
  using namespace Dune::XT::Functions;
  const auto grid_id = Dune::XT::Grid::bindings::grid_name<G>::value();
  const auto g_dim = G::dimension;

  bind_Spe10Model1Function<G, g_dim, 1, 1>(m, grid_id);
  bind_Spe10Model1Function<G, g_dim, g_dim, g_dim>(m, grid_id);
} // ... addbind_for_Grid(...)


template <class Tuple = Dune::XT::Grid::AvailableGridTypes>
void all_grids(pybind11::module& m)
{
  Dune::XT::Common::bindings::guarded_bind([&]() { //  different grids but same entity
    addbind_for_Grid<typename Tuple::head_type>(m);
  });
  all_grids<typename Tuple::tail_type>(m);
}


template <>
void all_grids<boost::tuples::null_type>(pybind11::module&)
{}


PYBIND11_MODULE(_spe10, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions._gridfunction_interface_1d");
  py::module::import("dune.xt.functions._gridfunction_interface_2d");
  py::module::import("dune.xt.functions._gridfunction_interface_3d");

  all_grids(m);
}
