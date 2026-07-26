// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/gridprovider/provider.hh>

#include <dune/gdt/spaces/hdiv/raviart-thomas.hh>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>
#include <python/gdt/dune/gdt/spaces/binding_helpers.hh>

namespace Dune {
namespace GDT {
namespace bindings {


/**
 * \note Assumes that GV is the leaf view!
 */
template <class GV, class R = double>
class RaviartThomasSpace
{
  using G = typename GV::Grid;
  static const size_t d = G::dimension;

public:
  using type = GDT::RaviartThomasSpace<GV, R>;
  using base_type = GDT::SpaceInterface<GV, d, 1, R>;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type
  bind(pybind11::module& m, const std::string& grid_id, const std::string& class_id = "raviart_thomas_space")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = space_class_name<d, R>(class_id, grid_id, /*always_append_range_dim=*/true);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](XT::Grid::GridProvider<G>& grid_provider, const int order, const std::string& logging_prefix) {
            return new type(grid_provider.leaf_view(), order, logging_prefix); // Otherwise we get an error here!
          }),
          // A space holds the grid *view*, which points into the grid the provider owns: without this, a space
          // built from a temporary provider outlives its grid and every later use is a use-after-free (#378).
          py::keep_alive<1, 2>(),
          "grid_provider"_a,
          "order"_a,
          "logging_prefix"_a = "");
    add_space_repr(c);

    const auto FactoryName = space_factory_name<R>(class_id);
    m.def(
        FactoryName.c_str(),
        [](XT::Grid::GridProvider<G>& grid, const int order, const std::string& logging_prefix) {
          return new type(grid.leaf_view(), order, logging_prefix); // Otherwise we get an error here!
        },
        py::keep_alive<0, 1>(), // see the init above
        "grid"_a,
        "order"_a,
        "logging_prefix"_a = "");

    return c;
  } // ... bind(...)
}; // class RaviartThomasSpace


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct RaviartThomasSpace_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  static const constexpr size_t d = G::dimension;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::RaviartThomasSpace;
    using Dune::XT::Grid::bindings::grid_name;

    RaviartThomasSpace<GV>::bind(m, grid_name<G>::value());

    RaviartThomasSpace_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct RaviartThomasSpace_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


PYBIND11_MODULE(_spaces_hdiv_raviart_thomas, m)
{
  namespace py = pybind11;
  using namespace Dune;
  using namespace Dune::XT;
  using namespace Dune::GDT;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  py::module::import("dune.gdt._spaces_interface");

  RaviartThomasSpace_for_all_grids<XT::Grid::bindings::AvailableGridTypes>::bind(m);
}
