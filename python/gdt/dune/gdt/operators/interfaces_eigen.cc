// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include <dune/xt/grid/grids.hh>
#include <python/xt/dune/xt/la/traits.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include "interfaces_all_grids.hh"


/// \todo Split like istl!

PYBIND11_MODULE(_operators_interfaces_eigen, m)
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

  // #if HAVE_EIGEN
  //   OperatorInterface_for_all_grids<LA::EigenDenseMatrix<double>,
  //                                   LA::bindings::Eigen,
  //                                   LA::bindings::Dense,
  //                                   XT::Grid::bindings::AvailableGridTypes>::bind(m, "eigen_dense");
  //   OperatorInterface_for_all_grids<LA::EigenRowMajorSparseMatrix<double>,
  //                                   LA::bindings::Eigen,
  //                                   LA::bindings::Sparse,
  //                                   XT::Grid::bindings::AvailableGridTypes>::bind(m, "eigen_sparse");
  // #endif
}
