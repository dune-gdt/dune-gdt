// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2018)
//   René Fritze     (2018)
//   Tobias Leibner  (2019)
//
// This file is part of the dune-pybindxi project:

/// \file
/// \brief Provides access to a Python interpreter (GlobalInterpreter()), embedding one where the process does not
///        already run one.

#ifndef DUNE_PYBINDXI_INTERPRETER_HH
#define DUNE_PYBINDXI_INTERPRETER_HH

#include <optional>
#include <string>

#include <dune/common/visibility.hh>

#include <pybind11/embed.h>

namespace Dune {
namespace PybindXI {


/// \brief Whether this process already runs a Python interpreter, i.e. whether we are being called from Python.
DUNE_EXPORT bool interpreter_is_running();


/**
 * \brief Gives access to a Python interpreter, embedding one if the process does not already run one.
 *
 * \note Most likely, you do not want to use this class directly, but GlobalInterpreter() instead!
 *
 * \note All members require the GIL and acquire it themselves. Any pybind11 object obtained from here must also be
 *       destroyed while holding the GIL; use pybind11::gil_scoped_acquire in the calling scope if you keep one alive
 *       beyond a single call.
 */
class DUNE_EXPORT ScopedInterpreter
{
public:
  /// \brief Embeds an interpreter, unless the process already runs one (in which case that one is used).
  ScopedInterpreter();

  pybind11::module import_module(const std::string& module_name);

private:
  /// Only engaged if we embedded the interpreter ourselves. Finalizing an interpreter we do not own (i.e. when we are
  /// called from Python) would tear down the caller's process, hence the optional.
  std::optional<pybind11::scoped_interpreter> interpreter_;
}; // class ScopedInterpreter


/// \brief Returns the process-wide ScopedInterpreter, creating it on first use.
DUNE_EXPORT ScopedInterpreter& GlobalInterpreter();


} // namespace PybindXI
} // namespace Dune

#endif // DUNE_PYBINDXI_INTERPRETER_HH
