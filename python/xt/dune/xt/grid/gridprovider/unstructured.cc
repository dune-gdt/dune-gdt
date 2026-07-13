// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/gridprovider/unstructured.hh>
#include <dune/xt/grid/grids.hh>

#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>

using namespace Dune;
using namespace Dune::XT::Grid::bindings;


// make_mixed_grid / make_prism_grid (dune/xt/grid/gridprovider/unstructured.hh) build meshes with
// more than one geometry type (or a prism), which only UGGrid can hold -- so they are bound for UG
// grids exclusively. The very same factories build the meshes used by the C++ space tests
// (dune/gdt/test/spaces/base.hh), so the bindings exercise identical meshes.
#if HAVE_DUNE_UGGRID || HAVE_UG


// Both factories share the same python signature -- (Dim, num_refinements) -> GridProvider -- so a
// single generic binder expresses it once; the concrete builder is passed as a function pointer.
template <class G, XT::Grid::GridProvider<G> (*factory_function)(unsigned int)>
void bind_unstructured_factory(pybind11::module& m, const std::string& name)
{
  using namespace pybind11::literals;
  m.def(
      name.c_str(),
      [](const Dimension<G::dimension>&, const unsigned int num_refinements) {
        return factory_function(num_refinements);
      },
      "dimension"_a,
      "num_refinements"_a = 0);
}


#endif // HAVE_DUNE_UGGRID || HAVE_UG


PYBIND11_MODULE(_grid_gridprovider_unstructured, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid._grid_gridprovider_provider");
  py::module::import("dune.xt.grid._grid_traits");

#if HAVE_DUNE_UGGRID || HAVE_UG
  bind_unstructured_factory<UG_2D, &XT::Grid::make_mixed_grid<UG_2D>>(m, "make_mixed_grid");
  bind_unstructured_factory<UG_3D, &XT::Grid::make_mixed_grid<UG_3D>>(m, "make_mixed_grid");
  bind_unstructured_factory<UG_3D, &XT::Grid::make_prism_grid<UG_3D>>(m, "make_prism_grid");
#endif
}
