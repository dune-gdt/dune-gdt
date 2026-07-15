// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include "config.h"

#include <string>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/functions/generic/grid-function.hh>

#include <python/xt/dune/xt/common/parameter.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/fmatrix.hh>
#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>
#include <python/xt/dune/xt/functions/class-name.hh>

namespace Dune::XT::Functions::bindings {


// Binds Functions::GenericGridFunction, evaluated *locally* on each element (in reference-element
// coordinates), with an optional post_bind callback invoked once per element during a grid walk
// (giving Python access to e.g. the element's geometry.center() or index, for element-dependent
// coefficients that GenericFunction's purely global evaluate cannot express). As with
// GenericFunction (generic-function.cc), the evaluate/post_bind/jacobian callbacks are plain
// std::function<...> slots, so pybind11/functional.h handles the Python-callable conversion and
// per-call GIL reacquisition without any explicit gil_scoped_acquire in this file.
template <class G, class E, size_t r = 1, size_t rC = 1, class R = double>
class GenericGridFunction
{
  using GP = XT::Grid::GridProvider<G>;

public:
  using type = Functions::GenericGridFunction<E, r, rC, R>;
  using base_type = Functions::GridFunctionInterface<E, r, rC, R>;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type bind(pybind11::module& m,
                         const std::string& grid_id = Grid::bindings::grid_name<G>::value(),
                         const std::string& layer_id = "",
                         const std::string& class_id = "generic_grid_function")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = grid_range_class_name<R>(class_id, grid_id, layer_id, r, rC);
    bound_type c(m, ClassName.c_str(), Common::to_camel_case(class_id).c_str());

    c.def(py::init([](int order, typename type::GenericEvaluateFunctionType evaluate, const std::string& name) {
            return new type(order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name);
          }),
          "order"_a,
          "evaluate"_a,
          "name"_a = type::static_id());
    c.def(py::init([](int order,
                      typename type::GenericPostBindFunctionType post_bind,
                      typename type::GenericEvaluateFunctionType evaluate,
                      const std::string& name) {
            return new type(order, post_bind, evaluate, Common::ParameterType{}, name);
          }),
          "order"_a,
          "post_bind"_a,
          "evaluate"_a,
          "name"_a = type::static_id());
    c.def(py::init([](int order,
                      typename type::GenericEvaluateFunctionType evaluate,
                      typename type::GenericJacobianFunctionType jacobian,
                      const std::string& name) {
            return new type(
                order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name, jacobian);
          }),
          "order"_a,
          "evaluate"_a,
          "jacobian"_a,
          "name"_a = type::static_id());

    // The overloads below differ only in the *content* of a std::function argument
    // (GenericEvaluateFunctionType's RangeReturnType), never in its C++ type signature as seen by
    // pybind11/functional.h's caster: any Python callable loads into any std::function<...> slot
    // regardless of what it returns. Without an explicit, concretely-typed dim_range tag argument,
    // pybind11 cannot distinguish the five (r, rC) registrations below and would always dispatch to
    // whichever was registered first -- so, as gridfunction.cc already does for its raw-lambda
    // factory, dim_range is threaded through here purely to disambiguate the overload set: as a
    // single Dimension<r> tag (only unambiguous for a vector-or-scalar range) ...
    if constexpr (rC == 1) {
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             const Grid::bindings::Dimension<r>& /*dim_range*/,
             const std::string& name) {
            return new type(order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name);
          },
          "grid"_a,
          "order"_a,
          "evaluate"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericPostBindFunctionType post_bind,
             typename type::GenericEvaluateFunctionType evaluate,
             const Grid::bindings::Dimension<r>& /*dim_range*/,
             const std::string& name) { return new type(order, post_bind, evaluate, Common::ParameterType{}, name); },
          "grid"_a,
          "order"_a,
          "post_bind"_a,
          "evaluate"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             typename type::GenericJacobianFunctionType jacobian,
             const Grid::bindings::Dimension<r>& /*dim_range*/,
             const std::string& name) {
            return new type(
                order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name, jacobian);
          },
          "grid"_a,
          "order"_a,
          "evaluate"_a,
          "jacobian"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
    }

    // ... or as a (Dimension<r>, Dimension<rC>) pair -- the general (matrix) case, plus the
    // degenerate scalar case (r == rC == 1): ConstantFunction/GridFunction accept a pair there too,
    // since dune-gdt code commonly tags tensor-like coefficients (e.g. a diffusion) with a
    // (dim_range, dim_range) pair uniformly across dimensions, including the 1x1 case.
    if constexpr (rC > 1 || r == 1) {
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             const std::pair<Grid::bindings::Dimension<r>, Grid::bindings::Dimension<rC>>& /*dim_range*/,
             const std::string& name) {
            return new type(order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name);
          },
          "grid"_a,
          "order"_a,
          "evaluate"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericPostBindFunctionType post_bind,
             typename type::GenericEvaluateFunctionType evaluate,
             const std::pair<Grid::bindings::Dimension<r>, Grid::bindings::Dimension<rC>>& /*dim_range*/,
             const std::string& name) { return new type(order, post_bind, evaluate, Common::ParameterType{}, name); },
          "grid"_a,
          "order"_a,
          "post_bind"_a,
          "evaluate"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
      m.def(
          Common::to_camel_case(class_id).c_str(),
          [](const GP& /*grid*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             typename type::GenericJacobianFunctionType jacobian,
             const std::pair<Grid::bindings::Dimension<r>, Grid::bindings::Dimension<rC>>& /*dim_range*/,
             const std::string& name) {
            return new type(
                order, type::default_post_bind_function(), evaluate, Common::ParameterType{}, name, jacobian);
          },
          "grid"_a,
          "order"_a,
          "evaluate"_a,
          "jacobian"_a,
          "dim_range"_a,
          "name"_a = type::static_id());
    }

    return c;
  }
}; // class GenericGridFunction


} // namespace Dune::XT::Functions::bindings


// Bound (r, rC) range combinations are intentionally the reduced {(1,1), (2,1), (2,2), (3,1),
// (3,3)} set (scalar / vector / square-matrix), not the full 9-combination set used by
// GridFunction/CheckerboardFunction: this header is entity-templated like those, so it is already
// instantiated once per bound grid type (dune/xt/grid/grids.bindings.hh), and #320's cost guardrail
// asks every WP to keep an eye on the resulting bindings build time and peak RAM. Widen this set if
// a use case needs e.g. a 1-to-2 or 2-to-3 GenericGridFunction.
template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct GenericGridFunction_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  using E = Dune::XT::Grid::extract_entity_t<GV>;

  static void bind(pybind11::module& m)
  {
    using Dune::XT::Functions::bindings::GenericGridFunction;

    GenericGridFunction<G, E, 1, 1>::bind(m);
    GenericGridFunction<G, E, 2, 1>::bind(m);
    GenericGridFunction<G, E, 2, 2>::bind(m);
    GenericGridFunction<G, E, 3, 1>::bind(m);
    GenericGridFunction<G, E, 3, 3>::bind(m);

    GenericGridFunction_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct GenericGridFunction_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


PYBIND11_MODULE(_functions_generic_grid_function, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.functions._functions_interfaces_grid_function_1d");
  py::module::import("dune.xt.functions._functions_interfaces_grid_function_2d");
  py::module::import("dune.xt.functions._functions_interfaces_grid_function_3d");

  GenericGridFunction_for_all_grids<>::bind(m);
} // PYBIND11_MODULE(...)
