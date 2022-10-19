// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2021)
//   René Fritze     (2022)
//
// This file is part of the dune-xt project:

#include "config.h"

#include <dune/xt/common/parameter.hh>

#include "parameter.hh"


PYBIND11_MODULE(_common_parameter, m)
{
  namespace py = pybind11;
  using type = Dune::XT::Common::ParametricInterface;

  py::class_<type> c(m, "ParametricInterface", "ParametricInterface");
  c.def_property_readonly("is_parametric", [](type& self) { return self.is_parametric(); });
  c.def("parameter_type", &type::parameter_type);
  c.def("parse_parameter", &type::parse_parameter);
}
