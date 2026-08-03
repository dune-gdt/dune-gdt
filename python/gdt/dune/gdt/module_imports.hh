// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_MODULE_IMPORTS_HH
#define PYTHON_DUNE_GDT_MODULE_IMPORTS_HH

#include <pybind11/pybind11.h>

// Import preamble shared by every binding module built ON TOP OF the operator stack (advection-fv,
// advection-dg, reconstruction, timestepper): the union of their dependencies. A superset is
// harmless -- importing an already-loaded module is a no-op, dune.gdt's __init__.py loads all of
// them anyway, and none of the modules listed here imports any of the four downstream modules back
// (so there are no cycles). Keeping ONE list instead of a near-identical block per module is what
// keeps SonarCloud's duplication gate quiet.
//
// NOT for use by the modules listed below themselves (a module importing itself would fail).
#define DUNE_GDT_BIND_OPERATOR_STACK_IMPORTS                                                                           \
  pybind11::module::import("dune.xt.common");                                                                          \
  pybind11::module::import("dune.xt.la");                                                                              \
  pybind11::module::import("dune.xt.grid");                                                                            \
  pybind11::module::import("dune.xt.functions");                                                                       \
                                                                                                                       \
  pybind11::module::import("dune.gdt._discretefunction_dof_vector");                                                   \
  pybind11::module::import("dune.gdt._discretefunction_discretefunction_1d");                                          \
  pybind11::module::import("dune.gdt._discretefunction_discretefunction_2d");                                          \
  pybind11::module::import("dune.gdt._discretefunction_discretefunction_3d");                                          \
  pybind11::module::import("dune.gdt._local_operators_element_interface");                                             \
  pybind11::module::import("dune.gdt._local_operators_intersection_interface");                                        \
  pybind11::module::import("dune.gdt._operators_interfaces_common");                                                   \
  pybind11::module::import("dune.gdt._operators_interfaces_eigen");                                                    \
  pybind11::module::import("dune.gdt._operators_interfaces_istl_1d");                                                  \
  pybind11::module::import("dune.gdt._operators_interfaces_istl_2d");                                                  \
  pybind11::module::import("dune.gdt._operators_interfaces_istl_3d");                                                  \
  pybind11::module::import("dune.gdt._operators_numerical_fluxes_1d");                                                 \
  pybind11::module::import("dune.gdt._operators_numerical_fluxes_2d");                                                 \
  pybind11::module::import("dune.gdt._operators_numerical_fluxes_3d");                                                 \
  pybind11::module::import("dune.gdt._operators_operator_1d");                                                         \
  pybind11::module::import("dune.gdt._operators_operator_2d");                                                         \
  pybind11::module::import("dune.gdt._operators_operator_3d");                                                         \
  pybind11::module::import("dune.gdt._spaces_interface")


#endif // PYTHON_DUNE_GDT_MODULE_IMPORTS_HH
