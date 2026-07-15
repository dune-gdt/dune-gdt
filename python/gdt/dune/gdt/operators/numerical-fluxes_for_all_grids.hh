// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_OPERATORS_NUMERICAL_FLUXES_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_OPERATORS_NUMERICAL_FLUXES_FOR_ALL_GRIDS_HH

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/local/numerical-fluxes/interface.hh>
#include <dune/gdt/local/numerical-fluxes/lax-friedrichs.hh>
#include <dune/gdt/local/numerical-fluxes/upwind.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

// Only the scalar (m == 1) case is bound: linear-transport and Burgers (#320 WP6's target C++ test
// setups) are both scalar conservation laws, and NumericalUpwindFlux itself is only implemented
// for m == 1 in C++ (it throws for m > 1, see ThisNumericalFluxIsNotAvailableForTheseDimensions in
// dune/gdt/local/numerical-fluxes/interface.hh). Vector-valued systems (e.g. Euler) are left to a
// follow-up.

namespace Dune {
namespace GDT {
namespace bindings {


template <class GV, class F = double>
class NumericalFluxInterface
{
  static const constexpr size_t d = GV::dimension;
  using I = XT::Grid::extract_intersection_t<GV>;

public:
  using type = GDT::NumericalFluxInterface<I, d, 1, F>;
  using bound_type = pybind11::class_<type>;
  using XIndependentFluxType = typename type::XIndependentFluxType;

  static std::string class_name(const std::string& grid_id, const std::string& class_id = "numerical_flux_interface")
  {
    return class_id + "_" + grid_id;
  }

  static bound_type bind(pybind11::module& m, const std::string& grid_id)
  {
    const auto ClassName = XT::Common::to_camel_case(class_name(grid_id));
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def_property_readonly("linear", &type::linear);
    c.def_property_readonly("x_dependent", &type::x_dependent);
    return c;
  } // ... bind(...)
}; // class NumericalFluxInterface


template <class GV, class F = double>
class NumericalUpwindFlux
{
  static const constexpr size_t d = GV::dimension;
  using I = XT::Grid::extract_intersection_t<GV>;
  using InterfaceType = NumericalFluxInterface<GV, F>;

public:
  using type = GDT::NumericalUpwindFlux<I, d, 1, F>;
  using base_type = typename InterfaceType::type;
  using bound_type = pybind11::class_<type, base_type>;
  using XIndependentFluxType = typename InterfaceType::XIndependentFluxType;

  static bound_type
  bind(pybind11::module& m, const std::string& grid_id, const std::string& class_id = "numerical_upwind_flux")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_id + "_" + grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](const XIndependentFluxType& flux) { return new type(flux); }), "flux"_a, py::keep_alive<1, 2>());

    // NOTE: the factory is registered under class_id (snake_case) as-is -- __init__.py's
    // _make_flux_dispatch looks up this exact (grid-agnostic) attribute name on the submodule.
    m.def(
        class_id.c_str(),
        [](const XIndependentFluxType& flux) { return new type(flux); },
        "flux"_a,
        py::keep_alive<0, 1>());
    return c;
  } // ... bind(...)
}; // class NumericalUpwindFlux


template <class GV, class F = double>
class NumericalLaxFriedrichsFlux
{
  static const constexpr size_t d = GV::dimension;
  using I = XT::Grid::extract_intersection_t<GV>;
  using InterfaceType = NumericalFluxInterface<GV, F>;

public:
  using type = GDT::NumericalLaxFriedrichsFlux<I, d, 1, F>;
  using base_type = typename InterfaceType::type;
  using bound_type = pybind11::class_<type, base_type>;
  using XIndependentFluxType = typename InterfaceType::XIndependentFluxType;

  static bound_type
  bind(pybind11::module& m, const std::string& grid_id, const std::string& class_id = "numerical_lax_friedrichs_flux")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_id + "_" + grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](const XIndependentFluxType& flux, const double lambda) { return new type(flux, lambda); }),
          "flux"_a,
          "lambda_"_a = 0.,
          py::keep_alive<1, 2>());

    // NOTE: see the analogous comment in NumericalUpwindFlux::bind above.
    m.def(
        class_id.c_str(),
        [](const XIndependentFluxType& flux, const double lambda) { return new type(flux, lambda); },
        "flux"_a,
        "lambda_"_a = 0.,
        py::keep_alive<0, 1>());
    return c;
  } // ... bind(...)
}; // class NumericalLaxFriedrichsFlux


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct NumericalFluxes_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::NumericalFluxInterface;
    using Dune::GDT::bindings::NumericalLaxFriedrichsFlux;
    using Dune::GDT::bindings::NumericalUpwindFlux;
    using Dune::XT::Grid::bindings::grid_name;

    NumericalFluxInterface<GV>::bind(m, grid_name<G>::value());
    NumericalUpwindFlux<GV>::bind(m, grid_name<G>::value());
    NumericalLaxFriedrichsFlux<GV>::bind(m, grid_name<G>::value());
    // add your extra dimensions here
    // ...
    NumericalFluxes_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct NumericalFluxes_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {} // recursion base case: no grid types left to bind
};


// Shared by numerical-fluxes_1d.cc/_2d.cc/_3d.cc: see DUNE_GDT_BIND_OPERATOR_MODULE (operator_for_all_grids.hh)
// for the rationale behind the per-dimension split.
#define DUNE_GDT_BIND_NUMERICAL_FLUXES_MODULE(dim)                                                                     \
  namespace py = pybind11;                                                                                             \
  using namespace Dune;                                                                                                \
                                                                                                                       \
  py::module::import("dune.xt.common");                                                                                \
  py::module::import("dune.xt.grid");                                                                                  \
  py::module::import("dune.xt.functions");                                                                             \
                                                                                                                       \
  NumericalFluxes_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                              \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_OPERATORS_NUMERICAL_FLUXES_FOR_ALL_GRIDS_HH
