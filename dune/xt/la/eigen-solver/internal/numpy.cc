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
  // Cached: this is queried once per quadrature point per element in some code paths (see
  // ElementwiseMinimumFunctionHelper), and neither the interpreter nor numpy can appear or disappear over the lifetime
  // of the process. Static initialization is thread-safe, so the probe runs exactly once even under a parallel walk.
  static const bool available = []() {
    try {
      // Constructing the global interpreter embeds one only if the process does not already run one, so this works
      // both from a standalone C++ program and from inside Python. It has to happen before the GIL is acquired --
      // there is nothing to acquire until an interpreter exists.
      auto& interpreter = PybindXI::GlobalInterpreter();
      // Held here rather than only inside import_module(), because the module it hands back is released again at the
      // end of this statement, and dropping a reference also requires the GIL.
      pybind11::gil_scoped_acquire acquire_gil;
      interpreter.import_module("numpy.linalg");
    } catch (...) {
      return false;
    }
    return true;
  }();
  return available;
} // ... numpy_eigensolver_available(...)


} // namespace Dune::XT::LA::internal
