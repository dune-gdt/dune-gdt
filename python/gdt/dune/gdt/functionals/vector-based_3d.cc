// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include "vector_based_for_all_grids.hh"

PYBIND11_MODULE(_functionals_vector_based_3d, m)
{
  namespace py = pybind11;
  using namespace Dune;
  using namespace Dune::XT;
  using namespace Dune::GDT;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  //  py::module::import("dune.gdt._local_functionals_element_interface");
  py::module::import("dune.gdt._functionals_interfaces_common");
  py::module::import("dune.gdt._functionals_interfaces_eigen");
  py::module::import("dune.gdt._functionals_interfaces_istl_1d");
  py::module::import("dune.gdt._functionals_interfaces_istl_2d");
  py::module::import("dune.gdt._functionals_interfaces_istl_3d");
  py::module::import("dune.gdt._spaces_interface");

  //  VectorBasedFunctional_for_all_grids<LA::CommonDenseVector<double>,
  //                               LA::bindings::Common,
  //                               XT::Grid::bindings::Available3dGridTypes>::bind(m, "common_dense");
  // #if HAVE_EIGEN
  //  VectorBasedFunctional_for_all_grids<LA::EigenDenseVector<double>,
  //                               LA::bindings::Eigen,
  //                               XT::Grid::bindings::Available3dGridTypes>::bind(m, "eigen_dense");
  // #endif
  VectorBasedFunctional_for_all_grids<LA::IstlDenseVector<double>,
                                      LA::bindings::Istl,
                                      XT::Grid::bindings::Available3dGridTypes>::bind(m, "istl");
  m.attr("__all__") = py::make_tuple();
}
