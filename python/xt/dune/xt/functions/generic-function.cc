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
#include <dune/xt/functions/generic/function.hh>

#include <python/xt/dune/xt/common/parameter.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/fmatrix.hh>
#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>

namespace Dune::XT::Functions::bindings {


// Binds Functions::GenericFunction, a *global* smooth function evaluated in physical (world)
// coordinates. Every callback slot is a std::function, so pybind11/functional.h converts a plain
// Python callable on construction; each subsequent call from a C++ grid walk reacquires the GIL
// automatically (that conversion is what pybind11 generates), so no explicit gil_scoped_acquire is
// required here -- see docs/source/example__custom_python_functions.md for a measurement of the
// resulting per-quadrature-point overhead relative to the parsed-expression ExpressionFunction.
template <size_t d, size_t r = 1, size_t rC = 1, class R = double>
class GenericFunction
{
  using type = Functions::GenericFunction<d, r, rC, R>;
  using base_type = Functions::FunctionInterface<d, r, rC, R>;
  using bound_type = pybind11::class_<type, base_type>;

public:
  static bound_type bind(pybind11::module& m, const std::string& class_id = "generic_function")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    std::string class_name = class_id + "_" + Common::to_string(d) + "_to_" + Common::to_string(r);
    if (rC > 1)
      class_name += "x" + Common::to_string(rC);
    class_name += "d";
    const auto ClassName = Common::to_camel_case(class_name);
    bound_type c(m, ClassName.c_str(), XT::Common::to_camel_case(class_id).c_str());

    c.def(py::init([](int order, typename type::GenericEvaluateFunctionType evaluate, const std::string& name) {
            return new type(order, evaluate, name);
          }),
          "order"_a,
          "evaluate"_a,
          "name"_a = type::static_id());
    c.def(py::init([](int order,
                      typename type::GenericEvaluateFunctionType evaluate,
                      typename type::GenericJacobianFunctionType jacobian,
                      const std::string& name) {
            return new type(order, evaluate, name, Common::ParameterType{}, jacobian);
          }),
          "order"_a,
          "evaluate"_a,
          "jacobian"_a,
          "name"_a = type::static_id());

    const auto FactoryName = XT::Common::to_camel_case(class_id);
    if (rC == 1)
      m.def(
          FactoryName.c_str(),
          [](Grid::bindings::Dimension<d> /*dim_domain*/,
             Grid::bindings::Dimension<r> /*dim_range*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             const std::string& name) { return new type(order, evaluate, name); },
          "dim_domain"_a,
          "dim_range"_a,
          "order"_a,
          "evaluate"_a,
          "name"_a = type::static_id());
    else
      m.def(
          FactoryName.c_str(),
          [](Grid::bindings::Dimension<d> /*dim_domain*/,
             std::pair<Grid::bindings::Dimension<r>, Grid::bindings::Dimension<rC>> /*dim_range*/,
             int order,
             typename type::GenericEvaluateFunctionType evaluate,
             const std::string& name) { return new type(order, evaluate, name); },
          "dim_domain"_a,
          "dim_range"_a,
          "order"_a,
          "evaluate"_a,
          "name"_a = type::static_id());

    return c;
  }
}; // class GenericFunction


} // namespace Dune::XT::Functions::bindings


PYBIND11_MODULE(_functions_generic_function, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.functions._functions_function_interface_1d");
  py::module::import("dune.xt.functions._functions_function_interface_2d");
  py::module::import("dune.xt.functions._functions_function_interface_3d");

  Dune::XT::Functions::bindings::GenericFunction<1, 1, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<1, 2, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<1, 2, 2>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<1, 3, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<1, 3, 3>::bind(m);

  Dune::XT::Functions::bindings::GenericFunction<2, 1, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<2, 2, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<2, 2, 2>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<2, 3, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<2, 3, 3>::bind(m);

  Dune::XT::Functions::bindings::GenericFunction<3, 1, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<3, 2, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<3, 2, 2>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<3, 3, 1>::bind(m);
  Dune::XT::Functions::bindings::GenericFunction<3, 3, 3>::bind(m);
} // PYBIND11_MODULE(...)
