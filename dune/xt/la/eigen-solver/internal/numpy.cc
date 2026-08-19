// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2020)

#include "config.h"

#include <dune/xt/common/interpreter.hh>

namespace Dune::XT::LA::internal {


bool numpy_eigensolver_available()
{
  // GlobalInterpreter() *embeds* an interpreter, which is only meaningful in a standalone C++ program. Called from
  // inside a running interpreter (i.e. through the Python bindings) pybind11's precheck_interpreter() rejects it with
  // pybind11_fail(), whose assert(!PyErr_Occurred()) touches the C-API -- and the bindings release the GIL around
  // grid walks (py::call_guard<py::gil_scoped_release>), so there is no thread state and PyErr_Occurred() segfaults.
  // Release builds only survive this because -DNDEBUG drops that assert. So do not go there at all when an
  // interpreter is already running; the answer is the same one the throwing path produced.
  //
  // The result is cached because this is called per quadrature point per element (see
  // ElementwiseMinimumFunctionHelper) and cannot change over the lifetime of the process.
  static const bool available = []() {
    if (Py_IsInitialized() != 0)
      return false;
    try {
      PybindXI::GlobalInterpreter().import_module("numpy.linalg");
    } catch (...) {
      return false;
    }
    return true;
  }();
  return available;
} // ... numpy_eigensolver_available(...)


} // namespace Dune::XT::LA::internal
