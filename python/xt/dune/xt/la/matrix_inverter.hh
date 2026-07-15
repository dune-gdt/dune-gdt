// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef DUNE_XT_LA_MATRIX_INVERTER_PBH
#define DUNE_XT_LA_MATRIX_INVERTER_PBH

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/la/container.bindings.hh>

#include <dune/xt/la/container.hh>
#include <dune/xt/la/type_traits.hh>
#include <dune/xt/la/matrix-inverter.hh>

namespace Dune::XT::LA {


/**
 * \brief Binds LA::MatrixInverter<M> (dune/xt/la/matrix-inverter.hh) for a matrix type M that
 *        already has a MatrixInverter specialization with C++ test coverage, including
 *        std::complex<double>-valued M (see dune/xt/test/la/matrixinverter_for_complex_matrix*.py).
 */
template <class M>
auto bind_MatrixInverter(pybind11::module& m)
{
  using C = MatrixInverter<M>;
  using Opts = MatrixInverterOptions<M>;

  namespace py = pybind11;
  using namespace pybind11::literals;

  const auto ClassName = Common::to_camel_case(bindings::container_name<M>::value() + "_matrix_inverter");

  py::class_<C> c(m, ClassName.c_str(), ClassName.c_str());

  c.def_static("types", &Opts::types);
  c.def_static("options", &Opts::options, "type"_a = "");

  c.def(py::init([](const M& matrix, const std::string& type) { return new C(matrix, type); }),
        "matrix"_a,
        "type"_a = "",
        py::keep_alive<1, 2>());
  c.def(py::init([](const M& matrix, const Common::Configuration& opts) { return new C(matrix, opts); }),
        "matrix"_a,
        "options"_a,
        py::keep_alive<1, 2>());

  c.def_property_readonly("options", &C::options);
  c.def_property_readonly("matrix", [](const C& self) { return M(self.matrix()); });
  c.def("inverse", [](const C& self) { return M(self.inverse()); });

  m.def(
      "make_matrix_inverter",
      [](const M& matrix, const std::string& type) { return C(matrix, type); },
      "matrix"_a,
      "type"_a = "",
      py::keep_alive<0, 1>());
  m.def(
      "make_matrix_inverter",
      [](const M& matrix, const Common::Configuration& opts) { return C(matrix, opts); },
      "matrix"_a,
      "options"_a,
      py::keep_alive<0, 1>());
  m.def(
      "invert_matrix",
      [](const M& matrix, const std::string& type) { return M(invert_matrix(matrix, type)); },
      "matrix"_a,
      "inversion_type"_a = "");
  m.def(
      "invert_matrix",
      [](const M& matrix, const Common::Configuration& opts) { return M(invert_matrix(matrix, opts)); },
      "matrix"_a,
      "inversion_options"_a);

  return c;
} // ... bind_MatrixInverter(...)


} // namespace Dune::XT::LA

#endif // DUNE_XT_LA_MATRIX_INVERTER_PBH
