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

#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>

#include <dune/gdt/local/integrands/div.hh>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>


namespace Dune {
namespace GDT {
namespace bindings {


/**
 * \sa GDT::LocalElementAnsatzValueTestDivProductIntegrand
 *
 * Pairs a d-valued test basis (its divergence) with a scalar ansatz basis, e.g. the velocity/pressure
 * coupling term of a mixed (Taylor-Hood-type) Stokes discretization. Unlike LocalElementProductIntegrand,
 * test and ansatz ranks are not equal, so there is a single instantiation per grid (test rank is always
 * the grid's dimension), not one per rank.
 */
template <class G, class E, class F = double>
class LocalElementAnsatzValueTestDivProductIntegrand
{
  static const constexpr size_t d = E::dimension;

public:
  using type = GDT::LocalElementAnsatzValueTestDivProductIntegrand<E, F, F, F>;
  using base_type = GDT::LocalBinaryElementIntegrandInterface<E, d, 1, F, F, 1, 1, F>;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type bind(pybind11::module& m,
                         const std::string& layer_id = "",
                         const std::string& grid_id = XT::Grid::bindings::grid_name<G>::value(),
                         const std::string& class_id = "local_element_ansatz_value_test_div_product_integrand")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    std::string class_name = class_id + "_" + grid_id + (layer_id.empty() ? "" : "_" + layer_id);
    if (!std::is_same<F, double>::value)
      class_name += "_" + XT::Common::Typename<F>::value(/*fail_wo_typeid=*/true);
    const auto ClassName = XT::Common::to_camel_case(class_name);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    // NOTE: unlike LocalElementProductIntegrand, this class has no logging_prefix constructor
    // argument (see div.hh), so none is exposed here.
    c.def(py::init([](const XT::Functions::GridFunctionInterface<E, 1, 1, F>& weight) { return new type(weight); }),
          "weight"_a);

    // factory
    const auto FactoryName = XT::Common::to_camel_case(class_id);
    m.def(
        FactoryName.c_str(),
        [](const XT::Functions::GridFunctionInterface<E, 1, 1, F>& weight) { return new type(weight); },
        "weight"_a);

    return c;
  } // ... bind(...)
}; // class LocalElementAnsatzValueTestDivProductIntegrand


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct LocalElementAnsatzValueTestDivProductIntegrand_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  using E = Dune::XT::Grid::extract_entity_t<GV>;
  static const constexpr size_t d = G::dimension;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::LocalElementAnsatzValueTestDivProductIntegrand;

    // Only meaningful once the test (divergenced) basis is vector-valued; in 1d it would coincide
    // with the already-bound scalar LocalElementProductIntegrand.
    if (d > 1)
      LocalElementAnsatzValueTestDivProductIntegrand<G, E>::bind(m);
    // add your extra dimensions here
    // ...
    LocalElementAnsatzValueTestDivProductIntegrand_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct LocalElementAnsatzValueTestDivProductIntegrand_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


PYBIND11_MODULE(_local_integrands_element_div, m)
{
  namespace py = pybind11;
  using namespace Dune;
  using namespace Dune::XT;
  using namespace Dune::GDT;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  py::module::import("dune.gdt._local_integrands_binary_element_interface");

  LocalElementAnsatzValueTestDivProductIntegrand_for_all_grids<XT::Grid::bindings::AvailableGridTypes>::bind(m);
}
