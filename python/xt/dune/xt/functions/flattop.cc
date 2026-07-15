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
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/functions/flattop.hh>

#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/fmatrix.hh>
#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>

namespace Dune::XT::Functions::bindings {


// FlatTopFunction is only implemented for the scalar case (r == rC == 1, see
// dune/xt/functions/flattop.hh); unlike the generic-function bindings, it takes no lambdas, but it
// is a useful test fixture for exactness properties (a C^infinity bump function with a known,
// closed-form value everywhere) to pair with the random Python callables in
// dune.xt.test.hypothesis_strategies.
template <size_t d, class R = double>
class FlatTopFunction
{
  using type = Functions::FlatTopFunction<d, 1, 1, R>;
  using base_type = Functions::FunctionInterface<d, 1, 1, R>;
  using bound_type = pybind11::class_<type, base_type>;

public:
  static bound_type bind(pybind11::module& m, const std::string& class_id = "flat_top_function")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = Common::to_camel_case(class_id + "_" + Common::to_string(d) + "d");
    bound_type c(m, ClassName.c_str(), XT::Common::to_camel_case(class_id).c_str());
    c.def(py::init<const typename type::DomainType&,
                   const typename type::DomainType&,
                   const typename type::DomainType&,
                   const typename type::RangeReturnType&,
                   const std::string>(),
          "lower_left"_a,
          "upper_right"_a,
          "boundary_layer"_a,
          "value"_a = typename type::RangeReturnType(1),
          "name"_a = type::static_id());

    m.def(
        XT::Common::to_camel_case(class_id).c_str(),
        [](const typename type::DomainType& lower_left,
           const typename type::DomainType& upper_right,
           const typename type::DomainType& boundary_layer,
           const typename type::RangeReturnType& value,
           const std::string& name) { return new type(lower_left, upper_right, boundary_layer, value, name); },
        "lower_left"_a,
        "upper_right"_a,
        "boundary_layer"_a,
        "value"_a = typename type::RangeReturnType(1),
        "name"_a = type::static_id());

    return c;
  }
}; // class FlatTopFunction


} // namespace Dune::XT::Functions::bindings


PYBIND11_MODULE(_functions_flattop, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.functions._functions_function_interface_1d");
  py::module::import("dune.xt.functions._functions_function_interface_2d");
  py::module::import("dune.xt.functions._functions_function_interface_3d");

  Dune::XT::Functions::bindings::FlatTopFunction<1>::bind(m);
  Dune::XT::Functions::bindings::FlatTopFunction<2>::bind(m);
  Dune::XT::Functions::bindings::FlatTopFunction<3>::bind(m);
} // PYBIND11_MODULE(...)
