// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2018)
//   Tobias Leibner  (2021)
//
// This file is part of the dune-xt project:
//
// This file is part of the dune-pybindxi project:

#include "interpreter.hh"

namespace Dune {
namespace PybindXI {


pybind11::module ScopedInterpreter::import_module(const std::string& module_name)
{
  auto result = modules_.find(module_name);
  if (result != modules_.end())
    return result->second;
  else {
    modules_[module_name] = pybind11::module::import(module_name.c_str());
    return modules_[module_name];
  }
} // ... load_module(...)


ScopedInterpreter& GlobalInterpreter()
{
  static ScopedInterpreter global_interpreter;
  return global_interpreter;
}


} // namespace PybindXI
} // namespace Dune
