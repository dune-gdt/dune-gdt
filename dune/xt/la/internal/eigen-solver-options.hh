// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2019)
//   René Fritze     (2018 - 2020)
//   Tobias Leibner  (2017 - 2020)

/// \file
/// \brief Option and type-list helpers shared by the eigen-solvers and the generalized eigen-solvers.

#ifndef DUNE_XT_LA_INTERNAL_EIGEN_SOLVER_OPTIONS_HH
#define DUNE_XT_LA_INTERNAL_EIGEN_SOLVER_OPTIONS_HH

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <dune/xt/common/configuration.hh>

namespace Dune::XT::LA::internal {


/**
 * \brief Assembles a list of solver types from the given candidates, keeping only the available ones.
 *
 * Each candidate is a pair of the type name and whether that backend is available, in order of preference:
\code
assemble_solver_types({{"lapack", Common::Lapacke::available()}, {"shifted_qr", true}});
\endcode
 */
static inline std::vector<std::string>
assemble_solver_types(std::initializer_list<std::pair<std::string, bool>> candidates)
{
  std::vector<std::string> ret;
  for (const auto& [type, is_available] : candidates)
    if (is_available)
      ret.emplace_back(type);
  return ret;
}


/**
 * \brief Returns true if type is one of available_types.
 * \note  The associated DUNE_THROW is left to the callers: the exception type has to be spelled out at the throwing
 *        site, since DUNE_THROW stringifies it into the message.
 */
static inline bool solver_type_available(const std::string& type, const std::vector<std::string>& available_types)
{
  for (const auto& tp : available_types)
    if (type == tp)
      return true;
  return false;
} // ... solver_type_available(...)


/**
 * \brief The default options shared by the eigen-solvers and the generalized eigen-solvers.
 * \note  Both users of this cache the result, this is not meant to be called in a hot path.
 */
static inline Common::Configuration common_eigen_solver_options(const bool compute_eigenvectors)
{
  Common::Configuration ret;
  ret["compute_eigenvalues"] = "true";
  ret["compute_eigenvectors"] = compute_eigenvectors ? "true" : "false";
  ret["check_for_inf_nan"] = "true";
  ret["real_tolerance"] = "1e-15"; // is only used if the respective assert_... is negative
  ret["assert_real_eigenvalues"] = "-1"; // if positive, this is the check tolerance
  ret["assert_positive_eigenvalues"] = "-1"; // if positive, this is the check tolerance
  ret["assert_negative_eigenvalues"] = "-1"; // if positive, this is the check tolerance
  ret["assert_real_eigenvectors"] = "-1"; // if positive, this is the check tolerance
  return ret;
} // ... common_eigen_solver_options(...)


/// \brief Returns the given default options with the "type" key set to the given type.
static inline Common::Configuration solver_options_with_type(const Common::Configuration& defaults,
                                                             const std::string& type)
{
  Common::Configuration opts = defaults;
  opts["type"] = type;
  return opts;
} // ... solver_options_with_type(...)


} // namespace Dune::XT::LA::internal

#endif // DUNE_XT_LA_INTERNAL_EIGEN_SOLVER_OPTIONS_HH
