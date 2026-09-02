// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2018)
//
// This file is part of the dune-pybindxi project:

#include "interpreter.hh"

namespace Dune {
namespace PybindXI {


bool interpreter_is_running()
{
  return Py_IsInitialized() != 0;
}


ScopedInterpreter::ScopedInterpreter()
{
  // When we are called from Python there already is an interpreter, and pybind11::scoped_interpreter refuses to
  // initialize a second one: precheck_interpreter() calls pybind11_fail(), whose assert(!PyErr_Occurred()) touches the
  // C-API. Since the bindings release the GIL around grid walks, that assert runs without a thread state and segfaults
  // (in release builds -DNDEBUG drops it and it merely throws). So embed one only if we actually need to; either way
  // the accessors below work the same afterwards.
  if (!interpreter_is_running())
    interpreter_.emplace();
}


pybind11::module ScopedInterpreter::import_module(const std::string& module_name)
{
  // Deliberately no module cache: pybind11::module::import is a sys.modules lookup, so caching buys next to nothing,
  // while a process-lifetime std::map of pybind11::module would be written from every thread of a parallel grid walk
  // and would release its references at static destruction time, possibly after the interpreter is gone.
  pybind11::gil_scoped_acquire acquire_gil;
  return pybind11::module::import(module_name.c_str());
} // ... import_module(...)


ScopedInterpreter& GlobalInterpreter()
{
  static ScopedInterpreter global_interpreter;
  return global_interpreter;
}


} // namespace PybindXI
} // namespace Dune
