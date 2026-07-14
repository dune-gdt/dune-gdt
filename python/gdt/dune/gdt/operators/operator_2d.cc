// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include "operator_for_all_grids.hh"

PYBIND11_MODULE(_operators_operator_2d, m)
{
  namespace py = pybind11;
  using namespace Dune;
  using namespace Dune::XT;
  using namespace Dune::GDT;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  py::module::import("dune.gdt._local_operators_element_interface");
  py::module::import("dune.gdt._local_operators_intersection_interface");
  py::module::import("dune.gdt._operators_interfaces_common");
  py::module::import("dune.gdt._operators_interfaces_eigen");
  py::module::import("dune.gdt._operators_interfaces_istl_1d");
  py::module::import("dune.gdt._operators_interfaces_istl_2d");
  py::module::import("dune.gdt._operators_interfaces_istl_3d");

  /// \todo Add other la backends if required
  Operator_for_all_grids<LA::IstlRowMajorSparseMatrix<double>,
                         LA::bindings::Istl,
                         XT::Grid::bindings::Available2dGridTypes>::bind(m, "istl_sparse");
  m.attr("__all__") = py::make_tuple();
}
