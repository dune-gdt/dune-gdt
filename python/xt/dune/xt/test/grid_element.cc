// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020 - 2021)
//   René Fritze     (2022)
//   Tobias Leibner  (2021)
//
// This file is part of the dune-xt project:

#include "config.h"

#include <dune/pybindxi/pybind11.h>
#include <dune/pybindxi/functional.h>

#include <python/xt/dune/xt/common/bindings.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>


PYBIND11_MODULE(_test_grid_element, m)
{
  namespace py = pybind11;
  using namespace pybind11::literals;
  using namespace Dune::XT;

  using G = YASP_2D_EQUIDISTANT_OFFSET;
  using E = Grid::extract_entity_t<typename G::LeafGridView>;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid");

  m.def("call_on_each_element", [](Grid::GridProvider<G>& grid, std::function<void(const E&)> generic_function) {
    auto gv = grid.leaf_view();
    for (auto&& element : elements(gv))
      generic_function(element);
  });
}
